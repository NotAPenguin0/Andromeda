#include <andromeda/app/application.hpp>

namespace andromeda {

Application::Application(int argc, char** argv) {
	log = std::make_unique<Log>();
	// Setup global logging system
	impl::_global_log_pointer = log.get();
	window = std::make_unique<Window>("Andromeda", 800, 600);
	graphics = gfx::Context::init(*window, *log);
}

int Application::run() {
	while (window->is_open()) {
		window->poll_events();
	}

	graphics->wait_idle();

	return 0;
}

} // namespace andromeda