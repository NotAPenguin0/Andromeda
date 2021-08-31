#include <andromeda/editor/widgets/input_text.hpp>

#include <andromeda/app/log.hpp>

namespace andromeda::editor {

namespace impl {

int input_event_callback_forward(ImGuiInputTextCallbackData* data) {
	InputText* widget = reinterpret_cast<InputText*>(data->UserData);
	return widget->event_callback(data);
}

}

InputText::InputText(std::string const& label, plib::bit_flag<Flags> flags, uint32_t bufsize) {
	this->label = label;	

	// If FixedBufferSize is not set, add the resize callback flag.
	if (!(flags & Flags::FixedBufferSize)) {
		flags |= ImGuiInputTextFlags_CallbackResize;
	}

	create_flags = flags;
	buffer.resize(bufsize);
	// Initialize the whole string with null terminators.
	std::fill(buffer.begin(), buffer.end(), '\0');
}

void InputText::display() {
	bool retval = false;
	if (create_flags & Flags::Multiline) {
		retval = ImGui::InputTextMultiline(label.c_str(), buffer.data(), buffer.size(), 
			ImVec2(0, 0), create_flags.value(), impl::input_event_callback_forward, this);
	}
	else {
		retval = ImGui::InputText(label.c_str(), buffer.data(), buffer.size(), create_flags.value(),
			impl::input_event_callback_forward, this);
	}

	// If SubmitWithEnter was specified, the return value of ImGui::InputText signals this event.
	if ((create_flags & Flags::SubmitWithEnter) && retval) {
		// If the history flag was set, store the submitted string in the history buffer
		if (create_flags & Flags::EnableHistory) {
			insert_in_history(buffer);
			// On submit we reset the current history pointer
			current_history_index = std::nullopt;
		}

		submit_callback(buffer);

		if (create_flags & Flags::FocusAfterSubmit) {
			ImGui::SetKeyboardFocusHere();
		}
	}
}

void InputText::on_submit(std::function<void(std::string const&)> callback) {
	submit_callback = std::move(callback);
}

void InputText::on_autocomplete(std::function<std::vector<std::string>(std::string const&)> callback) {
	autocomplete_callback = std::move(callback);
}

void InputText::clear() {
	std::fill(buffer.begin(), buffer.end(), '\0');
}


int InputText::event_callback(ImGuiInputTextCallbackData* data) {
	switch (data->EventFlag) {
	case ImGuiInputTextFlags_CallbackResize:
		resize_event(data);
		break;
	case ImGuiInputTextFlags_CallbackHistory:
		history_select_event(data);
		break;
	case ImGuiInputTextFlags_CallbackCompletion:
		autocomplete_event(data);
		break;
	}

	return 0;
}

void InputText::resize_event(ImGuiInputTextCallbackData* data) {
	if (create_flags & Flags::FixedBufferSize) {
		LOG_WRITE(LogLevel::Error, "Resize callback called for ImGui Text Input with FixedSize flag set");
	}

	buffer.resize(data->BufTextLen);
}

void InputText::history_select_event(ImGuiInputTextCallbackData* data) {
	if (!(create_flags & Flags::EnableHistory)) {
		LOG_WRITE(LogLevel::Error, "History callback called for ImGui Text Input without EnableHistory flag set");
	}

	std::optional<uint32_t> previous_index = current_history_index;
	// Go further in the past
	if (data->EventKey == ImGuiKey_UpArrow) {
		// There is no history being displayed yet, we select the most recent entry (only if there are entries).
		if (current_history_index == std::nullopt) {
			if (num_history_elements > 0) {
				current_history_index = (first_history_element + num_history_elements - 1) % history_buffer_size;
			}
		}
		// We are already displaying history, we need to select the previous entry (if there is one).
		else if (current_history_index != first_history_element) {
			current_history_index = current_history_index.value() - 1;
			// Wrap around to max index
			if (current_history_index.value() == -1) current_history_index = history_buffer_size - 1;
		}
	}
	// Go back to the present
	else if (data->EventKey == ImGuiKey_DownArrow) {
		// Can't go further if there is no history being displayed.
		if (current_history_index != std::nullopt) {
			uint32_t most_recent_index = (first_history_element + num_history_elements - 1) % history_buffer_size;
			// Already most recent, can't go further. Instead we disable history mode again (select none).
			if (current_history_index.value() == most_recent_index) {
				current_history_index = std::nullopt;
			}
			else {
				current_history_index = (current_history_index.value() + 1) % history_buffer_size;
			}
		}
	}
	// If the selected history element has changed we write new data to the input field.
	if (previous_index != current_history_index) {
		data->DeleteChars(0, data->BufTextLen);
		if (current_history_index != std::nullopt) {
			const char* history_str = history[current_history_index.value()].c_str();
			data->InsertChars(0, history_str);
		}
		else {
			data->InsertChars(0, "");
		}
	}
}

void InputText::autocomplete_event(ImGuiInputTextCallbackData* data) {
	std::vector<std::string> matches = autocomplete_callback(buffer);
	if (matches.size() == 0) {
		LOG_FORMAT(LogLevel::Debug, "No match for \'{}\'.", buffer);
	}
	else if (matches.size() == 1) {
		data->DeleteChars(0, data->CursorPos);
		data->InsertChars(0, matches.front().c_str());
	}
	else {
		// Find the length of the longest common substring of all strings, and complete to that.
		// Then display matches
		uint32_t match_len = 0;
		while (true) {
			bool all_match = true;
			char c = {};
			for (int i = 0; i < matches.size(); ++i) {
				std::string const& match = matches[i];
				if (i == 0) c = match[match_len];
				if (match_len >= match.size()) {
					all_match = false;
					break;
				}

				if (match[match_len] != c) {
					all_match = false;
					break;
				}
			}

			if (!all_match) break;
			match_len += 1;
		}

		data->DeleteChars(0, data->CursorPos);
		data->InsertChars(0, matches.front().c_str(), matches.front().c_str() + match_len);

		LOG_WRITE(LogLevel::Debug, "Possible matches: ");
		for (std::string const& match : matches) {
			LOG_FORMAT(LogLevel::Debug, "\t- {}", match);
		}
	}
}

void InputText::insert_in_history(std::string const& message) {
	if (num_history_elements == history_buffer_size) {
		history[first_history_element] = message;

		first_history_element = (first_history_element + 1) % history_buffer_size;
	} else {
		history[num_history_elements] = message;
		num_history_elements += 1;
	}
}

} // namespace andromeda::editor