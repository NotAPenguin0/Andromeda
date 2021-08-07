#include <andromeda/app/application.hpp>

#include <andromeda/assets/assets.hpp>

namespace andromeda {

Application::Application(int argc, char** argv) {
	log = std::make_unique<Log>();
	// Setup global logging system
	impl::_global_log_pointer = log.get();
	window = std::make_unique<Window>("Andromeda", 800, 600);
	// Note that we use thread_count() - 1 threads. The reason for this is that
	// the main thread is also included in thead_count(), but the task scheduler only uses
	// extra threads.
	scheduler = std::make_unique<thread::TaskScheduler>(std::thread::hardware_concurrency() - 1);
	graphics = gfx::Context::init(*window, *log, *scheduler);
	world = std::make_unique<World>();

	renderer = std::make_unique<gfx::Renderer>(*graphics);
}

int Application::run() {

	Handle<gfx::Texture> t1 = assets::load<gfx::Texture>(*graphics, "data/textures/aerial_rocks_04_diff_4k.tx");
	Handle<gfx::Texture> t2 = assets::load<gfx::Texture>(*graphics, "data/textures/aerial_rocks_04_disp_4k.tx");
	Handle<gfx::Texture> t3 = assets::load<gfx::Texture>(*graphics, "data/textures/aerial_rocks_04_rough_4k.tx");

	while (window->is_open()) {	
		window->poll_events();
		renderer->render_frame(*graphics, *world);
	}

	graphics->wait_idle();
	scheduler->shutdown();

	return 0;
}

} // namespace andromeda