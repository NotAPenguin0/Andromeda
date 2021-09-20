#include <andromeda/app/application.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/graphics/imgui.hpp>
#include <andromeda/components/mesh_renderer.hpp>
#include <andromeda/components/name.hpp>

#include <andromeda/components/camera.hpp>

#include <reflect/reflection.hpp>

namespace andromeda {

Application::Application(int argc, char** argv) {
	log = std::make_unique<Log>();
	// Setup global logging system
	impl::_global_log_pointer = log.get();
	window = std::make_unique<Window>("Andromeda", 1300, 800);
	window->maximize();
	// Note that we use thread_count() - 1 threads. The reason for this is that
	// the main thread is also included in thead_count(), but the task scheduler only uses
	// extra threads.
	scheduler = std::make_unique<thread::TaskScheduler>(std::thread::hardware_concurrency() - 1);
	graphics = gfx::Context::init(*window, *log, *scheduler);
	world = std::make_unique<World>();

	ImGui::CreateContext();
	
	editor = std::make_unique<editor::Editor>(*graphics, *window);
	renderer = std::make_unique<gfx::Renderer>(*graphics, *window);
}

int Application::run() {
	Handle<gfx::Mesh> m1 = assets::load<gfx::Mesh>(*graphics, "data/meshes/cube.mesh");

	ecs::entity_t cube = world->create_entity();
	{
		// Get thread-safe access to the ecs and add a MeshRenderer component pointing to mesh m1.
		thread::LockedValue<ecs::registry> ecs = world->ecs();
		ecs->add_component<MeshRenderer>(cube, m1);
		ecs->get_component<Name>(cube).name = "Parent cube";
	}

	ecs::entity_t cube2 = world->create_entity(cube);
	{
		thread::LockedValue<ecs::registry> ecs = world->ecs();
		ecs->add_component<MeshRenderer>(cube2, m1);
		auto& transform = ecs->get_component<Transform>(cube2);
		transform.position.z = 2.0f;
		transform.scale = glm::vec3(0.3f, 0.3f, 0.3f);
		ecs->get_component<Name>(cube2).name = "Child cube";
	}

	{
		ecs::entity_t cam = world->create_entity();
		thread::LockedValue<ecs::registry> ecs = world->ecs();
		ecs->add_component<Camera>(cam);

		Transform& trans = ecs->get_component<Transform>(cam);
		trans.position = glm::vec3(-3.0f, 0.0f, 0.0f);
		ecs->get_component<Name>(cam).name = "Main camera";

		renderer->create_viewport(100, 100, cam); // Will be resized anyway
	}

	while (window->is_open()) {	
		window->poll_events();
		gfx::imgui::new_frame();

		editor->update(*world, *graphics, *renderer);
		renderer->render_frame(*graphics, *world);
	}

	// We don't want the app to look like it's hanging while all resources are being freed, so 
	// hide the window.
	window->hide();

	graphics->wait_idle();

	assets::unload_all<gfx::Texture>(*graphics);
	assets::unload_all<gfx::Mesh>(*graphics);

	scheduler->shutdown();
	graphics->wait_idle();

	renderer->shutdown(*graphics);

	return 0;
}

} // namespace andromeda