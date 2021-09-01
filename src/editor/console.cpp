#include <andromeda/editor/console.hpp>

#include <andromeda/editor/style.hpp>
#include <andromeda/editor/widgets/table.hpp>
#include <andromeda/graphics/context.hpp>
#include <andromeda/graphics/imgui.hpp>

#include <filesystem>
namespace fs = std::filesystem;

namespace andromeda::editor {

static ImVec4 rgb_to_norm(int r, int g, int b) {
	return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

static ImVec4 log_level_color(LogLevel level) {
	// color values taken from https://devblogs.microsoft.com/commandline/updating-the-windows-console-colors/
	switch (level) {
	case LogLevel::Info:
		return ImVec4(1, 1, 1, 1);
	case LogLevel::Performance:
		return rgb_to_norm(155, 209, 247);
	case LogLevel::Debug:
		return rgb_to_norm(147, 147, 147);
	case LogLevel::Warning:
		return rgb_to_norm(243, 212, 101);
	case LogLevel::Error:
		return rgb_to_norm(233, 120, 124);
	case LogLevel::Fatal:
		return rgb_to_norm(243, 101, 101);
	}
}

Console::Console(gfx::Context& ctx, Window& window)
	: command_line("##console-cmd",
		plib::bit_flag<InputText::Flags>(InputText::Flags::SubmitWithEnter)
		| InputText::Flags::EnableCompletion | InputText::Flags::EnableHistory | InputText::Flags::FocusAfterSubmit) {

	// Show all log levels by default
	for (bool& lvl : show_levels) {
		lvl = true;
	}

	std::vector<CommandParser::Argument> load_asset_args = {
		{.name = "path", .description = "The path of the asset to load."},
	};

	command_parser.add_command("load-asset", [this, &ctx](std::vector<std::string> args) {
		fs::path path = args[1];
		std::string ext = path.extension().generic_string();
		if (ext == ".tx") {
			assets::load<gfx::Texture>(ctx, path.generic_string());
		}
		else if (ext == ".mesh") {
			assets::load<gfx::Mesh>(ctx, path.generic_string());
		}
		else {
			LOG_FORMAT(LogLevel::Error, "Unsupposed asset extension: {}", ext);
			LOG_WRITE(LogLevel::Info, "Note that all assets must be converted using the asset tool before loading.");
		}
		}, load_asset_args);

	command_parser.add_command("shutdown", [&window](auto args) {
		window.close();
	}, {});

	command_line.on_submit([this](std::string const& str) {
		command_parser.parse_and_execute(str);
		command_line.clear();
	});

	command_line.on_autocomplete([this](std::string const& str) -> std::vector<std::string> {
		return command_parser.get_commands().collect_with_prefix(str);
	});
}

void Console::write(LogLevel level, std::string_view str) {
	// Buffer is full, we need to replace a message
	if (num_messages == max_messages) {
		// Replace first message with new message and increase the first message index
		message_buffer[first_message] = Message{
			.level = level,
			.str = std::string{str}
		};

		first_message = (first_message + 1) % max_messages;
	}
	// Buffer is not full, we can simply add a message to the list
	else {
		message_buffer[first_message + num_messages] = Message{
			.level = level,
			.str = std::string{str}
		};
		num_messages += 1;
	}

	// New message added
	scroll_to_bottom = true;
}

void Console::clear() {
	// Note that this is enough to clear the log, since we'll simply overwrite the old
	// messages stored thanks to the algorithm in write().
	num_messages = 0;
	first_message = 0;
}

void Console::display() {
	if (ImGui::Begin(ICON_FA_CODE " Console", nullptr)) {
		// Console options
		ImGui::Checkbox("Auto-scroll##console", &auto_scroll);
		ImGui::SameLine();
		if (ImGui::Button("Clear##console")) clear();	
		for (int i = 0; i < show_levels.size(); ++i) {
			ImGui::SameLine();
			LogLevel lvl = static_cast<LogLevel>(i);
			std::string str = std::string(log_level_string(lvl)) + "##console";
			// Scroll to bottom when the checkbox was pressed, because new messages are visible.
			if (ImGui::Checkbox(str.c_str(), &show_levels[i])) {
				if (show_levels[i])
					scroll_to_bottom = true;
			}
		}

		ImGui::Separator();

		// Console table.
		{
			ImGuiTableFlags tbl_flags =
				ImGuiTableFlags_NoSavedSettings // There's no persistent data for this
				| ImGuiTableFlags_Borders // Display the borders inside the table for clarity
				| ImGuiTableFlags_ScrollY; // Allow scrolling inside the table.

			// Reserve some height for an input field for commands
			float const reserve_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
			ImVec2 tbl_size = ImVec2(0, -reserve_height);

			Table table{ "console-table", 2, tbl_flags, tbl_size };
			if (table.visible()) {
				// The column displaying message severity has a fixed width.
				static constexpr float sev_width = 80.0f;
				table.column("Severity##console-col", Table::ColumnFlags::locked, sev_width);
				table.column("Message##console-col", Table::ColumnFlags::locked);
				table.header_row();

				for (uint32_t i = 0; i < num_messages; ++i) {
					Message const& msg = message_buffer[(first_message + i) % max_messages];

					// Skip messages that should not be shown.
					if (!show_levels[static_cast<size_t>(msg.level)]) continue;

					table.next_column();
					auto color = style::ScopedStyleColor(ImGuiCol_Text, log_level_color(msg.level));
					std::string prefix = "[" + std::string(log_level_string(msg.level)) + "]";
					style::draw_text_centered(prefix, sev_width);

					table.next_column();
					ImGui::TextWrapped("%s", msg.str.c_str());
				}

				// If we need to autoscroll, set correct scroll
				if (scroll_to_bottom && auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
					scroll_to_bottom = false;
				}
			}
		}

		auto width = style::ScopedWidthModifier(ImGui::GetContentRegionAvail().x);
		command_line.display();
	}
	ImGui::End();
}

}