#pragma once

#include <phobos/log_interface.hpp>

#include <functional>
#include <string_view>
#include <mutex>

namespace andromeda {

/**
 * @enum LogLevel 
 * @brief Describes the log level of a message
*/
enum class LogLevel {
	Debug = 0, 
	Performance,
	Info,
	Warning,
	Error, 
	Fatal,
	MAX_ENUM_VALUE
};

/**
 * @brief Get the name of a LogLevel as a string.
 * @return The string of the enum value name.
*/
std::string_view log_level_string(LogLevel lvl);

/**
 * @var using log_write_fun = std::function<void(LogLevel, std::string_view)> 
 * @brief Callback type for the logging interface. Used to write the output to whatever output
 *		  stream you want.
*/
using log_write_fun = std::function<void(LogLevel, std::string_view)>;

/**
 * @class Log
 * @brief Manages all application logging and serves as an interface for Phobos to do logging from.
*/
class Log : public ph::LogInterface {
public:
	/**
	 * @var static log_write_fun stdout_log_func 
	 * @brief Default logging function pointing to stdout.
	*/
	static log_write_fun stdout_log_func;

	/**
	 * @brief Initializes the logging system with the default logging function pointing to stdout.
	*/
	Log() = default;

	/**
	 * @brief Sets the output function for the logger
	 * @param func A callback that will be called when logging. This callback function takes a
	 *		  LogLevel and a std::string_view as parameters.
	*/
	void set_output(log_write_fun func);

	/**
	 * @brief Callback for phobos logging messages.
	 * @param sev Log severity
	 * @param str Message to log
	*/
	void write(ph::LogSeverity sev, std::string_view str) override;

	/**
	 * @brief Writes a formatted output message
	 * @tparam ...Args Types of the format parameters
	 * @param lvl Logging level
	 * @param str Format string
	 * @param ...args Format parameters
	*/
	template<typename... Args>
	void write_format(LogLevel lvl, std::string_view str, Args&&... args) {
		write(lvl, fmt::vformat(str, fmt::make_format_args(std::forward<Args>(args)...)));
	}

	/**
	 * @brief Writes an output message.
	 * @param lvl Log level
	 * @param str Message to log
	*/
	void write(LogLevel lvl, std::string_view str);

private:
	std::mutex mutex{};
	log_write_fun output_func = stdout_log_func;
};

namespace impl {
	extern Log* _global_log_pointer;
}

#define LOG_WRITE(level, message) ::andromeda::impl::_global_log_pointer->write(level, message)
#define LOG_FORMAT(level, fmt_string, ...) ::andromeda::impl::_global_log_pointer->write_format(level, fmt_string, __VA_ARGS__)


} // namespace andromeda