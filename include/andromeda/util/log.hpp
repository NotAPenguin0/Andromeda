#ifndef ANDROMEDA_LOG_HPP_
#define ANDROMEDA_LOG_HPP_

#include <phobos/util/log_interface.hpp>

#include <string_view>

namespace andromeda::io {

class ConsoleLogger : public ph::log::LogInterface {
protected:
	// This function is thread safe
	void write(ph::log::Severity sev, std::string_view str) override;
};

ConsoleLogger& get_console_logger();

template<typename... Ts>
void log(ph::log::Severity sev, std::string_view fmt, Ts&&... args) {
	get_console_logger().write_fmt(sev, fmt, std::forward<Args>(args)...);
}

template<typename... Ts>
void log(std::string_view fmt, Ts&&... args) {
	log(ph::log::Severity::Debug, fmt, std::forward<Args>(args) ...);
}

}

#endif