#pragma once

#include <andromeda/app/log.hpp>
#include <andromeda/editor/command_parser.hpp>
#include <andromeda/editor/widgets/input_text.hpp>
#include <andromeda/graphics/forward.hpp>

#include <plib/trie.hpp>

#include <array>
#include <string>

namespace andromeda::editor {

/**
 * @class Console 
 * @brief Editor widget for displaying logging messages and executing commands.
*/
class Console {
public:

	/**
	 * @brief Initializes the console widget.
	 * @param ctx Reference to the graphics context
	 * @param window Reference to the application window.
	*/
	Console(gfx::Context& ctx, Window& window);

	/**
	 * @brief The maximum amount of messages that will be stored and displayed in the console.
	*/
	static constexpr uint32_t max_messages = 100;

	/**
	 * @brief Writes a message to the console. Only the 100 most recent messages will be displayed.
	 * @param level Logging level of the message. This determines color and prefix.
	 * @param msg String with the message to display. Must be null-terminated.
	*/
	void write(LogLevel level, std::string_view msg);

	/**
	 * @brief Clears the log, removing all messages.
	*/
	void clear();

	/**
	 * @brief Displays the console UI.
	*/
	void display();

private:
	struct Message {
		LogLevel level;
		std::string str;
	};

	/**
	 * @brief Ring buffer storing all messages to display.
	*/
	std::array<Message, max_messages> message_buffer;

	/**
	 * @brief Index of the first (and oldest) message. When the buffer is full, we increment this to 
	 *		  essentially remove it from the message list.
	*/
	uint32_t first_message = 0;

	/**
	 * @brief The amount of currently stored messages. Once this reaches the max capacity every new message will
	 *		  pop the oldest message from the list.
	*/
	uint32_t num_messages = 0;

	/**
	 * @brief Whether to automatically scroll to the bottom when a new logging message is received.
	*/
	bool auto_scroll = true;

	/**
	 * @brief Indicates that the UI should scroll to the bottom because a new entry was added and auto_scroll
	 *		  is on.
	*/
	bool scroll_to_bottom = false;

	/**
	 * @brief Stores an entry for each log level indicating whether to show or hide messages of this level.
	*/
	std::array<bool, static_cast<size_t>(LogLevel::MAX_ENUM_VALUE)> show_levels{};

	/**
	 * @brief InputText widget that displays the command line.
	*/
	InputText command_line;

	/**
	 * @brief Stores and processes all commands.
	*/
	CommandParser command_parser;
};

} // namespace andromeda::editor