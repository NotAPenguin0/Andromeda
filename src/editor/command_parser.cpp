#include <andromeda/editor/command_parser.hpp>

#include <andromeda/util/string_ops.hpp>
#include <andromeda/app/log.hpp>

namespace andromeda::editor {

plib::trie<std::string, CommandParser::CommandStorage> const& CommandParser::get_commands() const {
	return commands;
}

void CommandParser::add_command(std::string const& name, command_func callback, std::vector<Argument> arguments) {
	CommandStorage storage;
	storage.callback = std::move(callback);
	storage.arguments = std::move(arguments);
	storage.max_num_args = storage.arguments.size();
	for (Argument const& arg : storage.arguments) {
		if (!arg.optional) storage.num_required_args += 1;
	}
	commands.insert(name, std::move(storage));
}

void CommandParser::parse_and_execute(std::string const& cmd_line) const {
	// We first split the string in a list of substrings by splitting on spaces.
	args_type parsed = str::split(cmd_line, ' ');
	if (parsed.empty()) {
		return;
	}

	std::string const& command = parsed[0];
	
	std::optional<CommandStorage> cmd = commands.get(command);
	if (!cmd) {
		LOG_FORMAT(LogLevel::Error, "Unrecognized command: \'{}\'", command);
		return;
	}

	// Verify argument usage.

	uint32_t const num_args = parsed.size() - 1u;

	if (num_args < cmd.value().num_required_args) {
		LOG_FORMAT(LogLevel::Error, "Not enough arguments for command {}. Expected {} arguments, got {}",
			command, cmd.value().num_required_args, num_args);
		log_command_usage(command, cmd.value());
		return;
	}
	if (num_args > cmd.value().max_num_args) {
		LOG_FORMAT(LogLevel::Error, "Too many arguments for command {}. Expected {} arguments, got {}",
			command, cmd.value().max_num_args, num_args);
		log_command_usage(command, cmd.value());
		return;
	}

	LOG_FORMAT(LogLevel::Debug, "Executing command: {}", cmd_line);
	cmd.value().callback(parsed);
}

void CommandParser::log_command_usage(std::string const& command, CommandStorage const& info) const {
	LOG_FORMAT(LogLevel::Error, "Usage for command {}: {} {}", command, command, fmt::join(info.arguments, " "));
	for (auto const& arg : info.arguments) {
		LOG_FORMAT(LogLevel::Error, "\t{} - {}", arg, arg.description);
	}
}

}