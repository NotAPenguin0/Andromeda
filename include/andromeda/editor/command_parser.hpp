#pragma once

#include <plib/trie.hpp>

#include <functional>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace andromeda::editor {

/**
 * @class CommandParser 
 * @brief Class used to parse, store and process commands.
*/
class CommandParser {
public:

    /**
     * @brief An instance of this will be passed to the command callback.
     *		  The first argument is the command itself.
    */
    using args_type = std::vector<std::string>;

    /**
     * @brief Callback function for a command. Takes in an argument list.
    */
    using command_func = std::function<void(args_type)>;

    /**
     * @brief Describes a single argument
    */
    struct Argument {

        /**
         * @brief Shorthand name for the argument.
        */
        std::string name{};

        /**
         * @brief Longer description of the argument.
        */
        std::string description{};

        /**
         * @brief Whether this argument is optional or required.
         *		  Note that all optional arguments must be at the end of the argument list.
        */
        bool optional = false;
    };

    /**
     * @brief This struct stores a command with all associated information.
    */
    struct CommandStorage {
        command_func callback;
        std::vector<Argument> arguments;
        uint32_t num_required_args = 0;
        uint32_t max_num_args = 0;
    };

    /**
     * @brief Get the list of commands with callbacks.
     * @return A plib::trie that stores a mapping from command name -> command.
    */
    plib::trie<std::string, CommandStorage> const& get_commands() const;

    /**
     * @brief Adds a command to the parser.
     * @param name Name of the command.
     * @param callback Callback to call when the command is executed.
    */
    void add_command(std::string const& name, command_func callback, std::vector<Argument> arguments);

    /**
     * @brief Parse a command line into a command + list of arguments and execute it.
     * @param cmd_line String describing the command line.
    */
    void parse_and_execute(std::string const& cmd_line) const;

private:
    plib::trie<std::string, CommandStorage> commands;

    void log_command_usage(std::string const& command, CommandStorage const& info) const;
};

} // namespace andromeda::editor

template<>
struct fmt::formatter<andromeda::editor::CommandParser::Argument> {

    // We do not support parsing
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(andromeda::editor::CommandParser::Argument const& arg, FormatContext& ctx) -> decltype(ctx.out()) {
        if (arg.optional) {
            return fmt::format_to(ctx.out(), "[{}]", arg.name);
        }

        return fmt::format_to(ctx.out(), "{}", arg.name);
    }
};