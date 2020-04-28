#include <andromeda/util/log.hpp>

#include <iostream>
#include <mutex>

namespace safe_io {

// If you get this struct from a ProtectedStream::acquire(), access to the contained stream object is guaranteed to be thread safe.
template<typename Stream>
struct LockedStream {
	std::lock_guard<std::mutex> _lock;
	Stream& stream;
};

// Holds an input/output stream so it can be used concurrently. To get access to the stream, call acquire(). 
template<typename Stream>
class ProtectedStream {
public:
	ProtectedStream(Stream& s) : stream(s) {}

	LockedStream<Stream> acquire() {
		return { ._lock = std::lock_guard(mut), .stream = stream };
	}

private:
	std::mutex mut;
	Stream& stream;
};

static ProtectedStream cout(std::cout);
static ProtectedStream cerr(std::cerr);

}

namespace andromeda::io {

static std::string_view severity_string(ph::log::Severity sev) {
	switch (sev) {
	case ph::log::Severity::Debug:
		return "[DEBUG]";
	case ph::log::Severity::Info:
		return "[INFO]";
	case ph::log::Severity::Warning:
		return "[WARNING]";
	case ph::log::Severity::Error:
		return "[ERROR]";
	case ph::log::Severity::Fatal:
		return "[FATAL ERROR]";
	}
}

void ConsoleLogger::write(ph::log::Severity sev, std::string_view str) {
	auto [_, out] = safe_io::cout.acquire();
	out << severity_string(sev) << ": " << str << "\n";
}

ConsoleLogger& get_console_logger() {
	static ConsoleLogger logger;
	return logger;
}

}