#include <andromeda/app/application.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/graphics/imgui.hpp>

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

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	renderer = std::make_unique<gfx::Renderer>(*graphics, *window);
}

int Application::run() {
	Handle<gfx::Texture> t1 = assets::load<gfx::Texture>(*graphics, "data/textures/aerial_rocks_04_diff_4k.tx");
	Handle<gfx::Texture> t2 = assets::load<gfx::Texture>(*graphics, "data/textures/aerial_rocks_04_disp_4k.tx");
	Handle<gfx::Texture> t3 = assets::load<gfx::Texture>(*graphics, "data/textures/aerial_rocks_04_rough_4k.tx");

	Handle<gfx::Mesh> m1 = assets::load<gfx::Mesh>(*graphics, "data/meshes/cube.mesh");
	Handle<gfx::Mesh> m2 = assets::load<gfx::Mesh>(*graphics, "data/meshes/plane.mesh");

	while (window->is_open()) {	
		window->poll_events();
		gfx::imgui::new_frame();

		if (ImGui::Begin("Window")) {
			ImGui::Button("Helo", ImVec2(100, 20));
		}
		ImGui::End();

		if (ImGui::Begin("Window2")) {
			ImGui::Button("Hiya", ImVec2(50, 30));
		}
		ImGui::End();

		renderer->render_frame(*graphics, *world);
	}

	// We don't want the app to look like it's hanging while all resources are being freed, so 
	// hide the window.
	window->hide();

	assets::unload_all<gfx::Texture>(*graphics);
	assets::unload_all<gfx::Mesh>(*graphics);

	scheduler->shutdown();
	graphics->wait_idle();

	renderer->shutdown(*graphics);

	return 0;
}

} // namespace andromeda