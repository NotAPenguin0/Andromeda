#include <andromeda/app/log.hpp>

#include <color.hpp>

#include <iostream>

namespace andromeda {

std::string_view log_level_string(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Debug:
            return "Debug";
        case LogLevel::Performance:
            return "Performance";
        case LogLevel::Info:
            return "Info";
        case LogLevel::Warning:
            return "Warning";
        case LogLevel::Error:
            return "Error";
        case LogLevel::Fatal:
            return "Fatal";
        default:
            throw std::runtime_error("Invalid logging level");
    }
}

static std::ostream& dye_log_level(LogLevel lvl, std::ostream& out) {
    auto string = log_level_string(lvl);
    switch (lvl) {
        case LogLevel::Debug:
            return out << hue::light_green << "[" << string << "]";
        case LogLevel::Performance:
            return out << hue::light_aqua << "[" << string << "]";
        case LogLevel::Info:
            return out << hue::white << "[" << string << "]";
        case LogLevel::Warning:
            return out << hue::light_yellow << "[" << string << "]";
        case LogLevel::Error:
            return out << hue::light_red << "[" << string << "]";
        case LogLevel::Fatal:
            return out << hue::red << "[" << string << "]";
        default:
            throw std::runtime_error("Invalid logging level");
    }
}

static LogLevel ph_to_log_level(ph::LogSeverity sev) {
    switch (sev) {
        case ph::LogSeverity::Info:
            return LogLevel::Info;
        case ph::LogSeverity::Debug:
            return LogLevel::Debug;
        case ph::LogSeverity::Warning:
            return LogLevel::Warning;
        case ph::LogSeverity::Error:
            return LogLevel::Error;
        case ph::LogSeverity::Fatal:
            return LogLevel::Fatal;
        default:
            throw std::runtime_error("Invalid logging level");
    }
}

// Initialize the default logger
log_write_fun Log::stdout_log_func = [](LogLevel lvl, std::string_view msg) {
    dye_log_level(lvl, std::cout) << ": " << msg << hue::reset << "\n";
};

void Log::set_output(log_write_fun func) {
    std::lock_guard lock(mutex);
    output_func = func;
}


void Log::write(ph::LogSeverity sev, std::string_view str) {
    // Do not buffer phobos output, as it's often validation layer error messages that we want to see right away.
    write_now(ph_to_log_level(sev), str);
//	write(ph_to_log_level(sev), str);
}

void Log::write(LogLevel lvl, std::string_view str) {
    mutex.lock();

    // We need a flush
    if (buffer_index >= bufsize) {
        mutex.unlock(); // Unlock before calling flush as flush() locks too
        flush();
        mutex.lock();
    }
    uint32_t const index = buffer_index++;
    // Store new entry in buffer and increment index
    message_buffer[index] = BufferedMessage{.lvl = lvl, .str = std::string{str}};
    mutex.unlock();
}

void Log::write_now(LogLevel lvl, std::string_view str) {
    std::lock_guard lock(mutex);
    write_unlocked(lock, lvl, str);
    std::cout << std::flush;
}

void Log::flush() {
    // Note that during a flush no messages can be written.
    std::lock_guard lock(mutex);
    // Write out messages from oldest to newest and reset buffer index
    for (uint32_t i = 0; i < buffer_index; ++i) {
        output_func(message_buffer[i].lvl, message_buffer[i].str);
    }
    buffer_index = 0;
}

void Log::write_unlocked(std::lock_guard<std::mutex> const& lock, LogLevel lvl, std::string_view str) {
    output_func(lvl, str);
}

namespace impl {
// Define _global_log_pointer
Log* _global_log_pointer = nullptr;
}

} // namespace andromeda