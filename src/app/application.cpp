#include <andromeda/app/application.hpp>

namespace andromeda {

Application::Application(int argc, char** argv) {
	log = std::make_unique<Log>();
	// Setup global logging system
	impl::_global_log_pointer = log.get();
	window = std::make_unique<Window>("Andromeda", 800, 600);
	graphics = gfx::Context::init(*window, *log);
	world = std::make_unique<World>();
	scheduler = std::make_unique<thread::TaskScheduler>(graphics->thread_count());
}

int Application::run() {
	thread::task_id first = scheduler->schedule([](uint32_t thread_index) {
		LOG_FORMAT(LogLevel::Info, "Thread 0 sleeping");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		LOG_FORMAT(LogLevel::Info, "Thread 0 waking up");
	});
	for (int i = 1; i < graphics->thread_count(); ++i) {
		scheduler->schedule([i](uint32_t thread_index) {
			LOG_FORMAT(LogLevel::Info, "Thread {} sleeping", i);
			std::this_thread::sleep_for(std::chrono::seconds(i));
			LOG_FORMAT(LogLevel::Info, "Thread {} waking up", i);
		}, { first });
	}

	while (window->is_open()) {	
		window->poll_events();
	}

	graphics->wait_idle();
	scheduler->shutdown();

	// Should report an error
	scheduler->schedule([](uint32_t) {});

	return 0;
}

} // namespace andromeda