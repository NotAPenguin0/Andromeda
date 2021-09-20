#pragma once

#include <andromeda/graphics/imgui.hpp>

#include <string>
#include <vector>
#include <plib/bit_flag.hpp>
#include <functional>
#include <array>

namespace andromeda::editor {

namespace impl {

int input_event_callback_forward(ImGuiInputTextCallbackData* data);

}

/**
 * @class InputText 
 * @brief Wrapper class around ImGui::InputText and ImGui::InputTextMultiline.
 *		  Also supports autocompletion and manages input history.
 *		  Note that this is a widget that needs to be kept alive, since it stores a 
 *	      modifiable input buffer. Do not create these on the fly every frame when building the UI.
*/
class InputText {
public:
	enum class Flags {
		None = ImGuiInputTextFlags_None,
		AllowDecimal = ImGuiInputTextFlags_CharsDecimal,
		AllowHex = ImGuiInputTextFlags_CharsHexadecimal,
		AllowScientific = ImGuiInputTextFlags_CharsScientific,
		Uppercase = ImGuiInputTextFlags_CharsUppercase,
		NoBlank = ImGuiInputTextFlags_CharsNoBlank,
		AutoSelectAll = ImGuiInputTextFlags_AutoSelectAll,
		SubmitWithEnter = ImGuiInputTextFlags_EnterReturnsTrue,
		EnableCompletion = ImGuiInputTextFlags_CallbackCompletion,
		EnableHistory = ImGuiInputTextFlags_CallbackHistory,
		AllowTabInput = ImGuiInputTextFlags_AllowTabInput,
		CtrlEnterNewLine = ImGuiInputTextFlags_CtrlEnterForNewLine,
		AlwaysOverwrite = ImGuiInputTextFlags_AlwaysOverwrite,
		ReadOnly = ImGuiInputTextFlags_ReadOnly,
		Password = ImGuiInputTextFlags_Password,
		NoUndoRedo = ImGuiInputTextFlags_NoUndoRedo,

		// Additional flags not present in ImGui
		Multiline = 1 << 20,
		FixedBufferSize = 1 << 21,
		FocusAfterSubmit = 1 << 22 // Automatically re-focus the widget after submitting.
	};

	/**
	 * @brief How many bytes to reserve initially by default. 
	 *		  You can change this value by passing in a different parameter for bufsize in the constructor.
	*/
	static constexpr uint32_t initial_buffer_size = 256;

	/**
	 * @brief Size of the history buffer.
	*/
	static constexpr uint32_t history_buffer_size = 100;

	/**
	 * @brief Creates an InputText widget.
	 * @param label Unique label for the InputText widget.
	 * @param flags Creation flags for the InputText widget.
	 * @param bufsize maximum buffer size. If flags includes Flags::FixedBufferSize this parameter
	 *				  indicates the maximum buffer size. Otherwise this parameter indicates the intial 
	 *				  reserved buffer size (default value is 256 characters).
	*/
	InputText(std::string const& label, plib::bit_flag<Flags> flags, uint32_t bufsize = initial_buffer_size);

	InputText(InputText const& rhs) = delete;
	InputText(InputText&& rhs) = delete;

	InputText& operator=(InputText const&) = delete;
	InputText& operator=(InputText&&) = delete;

	/**
	 * @brief Displays the InputText UI.
	*/
	void display();

	/**
	 * @brief Called when Flags::SubmitWithEnter is set and the Enter key is hit.
	 * @param callback Callback to call when receiving a submitted string.
	*/
	void on_submit(std::function<void(std::string const&)> callback);

	/**
	 * @brief Called when Flags::EnableCompletion is set and the Tab key is hit.
	 * @param callback Callback that returns completion candidates for a given input string.
	*/
	void on_autocomplete(std::function<std::vector<std::string>(std::string const&)> callback);

	/**
	 * @brief Clears the input buffer.
	*/
	void clear();

private:

	/**
	 * @brief UI label given to the widget.
	*/
	std::string label;

	/**
	 * @brief The input buffer for the input text field.
	*/
	std::string buffer;

	/**
	 * @brief Flags used when creating this widget.
	*/
	plib::bit_flag<Flags> create_flags = {};

	/**
	 * @brief Called on submit.
	*/
	std::function<void(std::string const&)> submit_callback;

	/**
	 * @brief Function that will return autocompletion candidates for a given input string.
	*/
	std::function<std::vector<std::string>(std::string const&)> autocomplete_callback;

	/**
	 * @brief History buffer. Together with the two variables below this serves as a ring buffer.
	*/
	std::array<std::string, history_buffer_size> history;

	/**
	 * @brief Index of the first stored element in the history buffer.
	*/
	uint32_t first_history_element = 0;

	/**
	 * @brief Amount of currently stored history elements.
	*/
	uint32_t num_history_elements = 0;

	/**
	 * @brief If there is currently a history element selected, this stores its index.
	*/
	std::optional<int> current_history_index = std::nullopt;

	// This function needs to access the private event_callback() function.
	friend int impl::input_event_callback_forward(ImGuiInputTextCallbackData*);

	/**
	 * @brief Callback for imgui.
	*/
	int event_callback(ImGuiInputTextCallbackData* data);

	/**
	 * @brief Handle buffer resize event
	*/
	void resize_event(ImGuiInputTextCallbackData* data);

	/**
	 * @brief Handle the history event.
	*/
	void history_select_event(ImGuiInputTextCallbackData* data);

	/**
	 * @brief Handle the autocomplete event.
	*/
	void autocomplete_event(ImGuiInputTextCallbackData* data);

	/**
	 * @brief Inserts a new message into the history buffer. May erase old history.
	 * @param message Message to insert.
	*/
	void insert_in_history(std::string const& message);
};

} // namespace andromeda::editor