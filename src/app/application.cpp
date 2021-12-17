#include <andromeda/app/application.hpp>


#include <andromeda/components/mesh_renderer.hpp>
#include <andromeda/components/name.hpp>
#include <andromeda/components/point_light.hpp>

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
    Handle<gfx::Mesh> m2 = assets::load<gfx::Mesh>(*graphics, "data/meshes/sphere.mesh");

	Handle<gfx::Material> mat = assets::load<gfx::Material>(*graphics, "data/materials/pbr.mat");

	ecs::entity_t sphere = world->create_entity();
	{
		// Get thread-safe access to the ecs and add a MeshRenderer component pointing to mesh m1.
		thread::LockedValue<ecs::registry> ecs = world->ecs();
		ecs->add_component<MeshRenderer>(sphere, m2, mat);
		ecs->get_component<Name>(sphere).name = "Parent sphere";
	}

	ecs::entity_t sphere2 = world->create_entity(sphere);
	{
		thread::LockedValue<ecs::registry> ecs = world->ecs();
		ecs->add_component<MeshRenderer>(sphere2, m2, mat);
		auto& transform = ecs->get_component<Transform>(sphere2);
		transform.position.z = 2.0f;
		transform.scale = glm::vec3(0.3f, 0.3f, 0.3f);
		ecs->get_component<Name>(sphere2).name = "Child sphere";
	}

	{
		ecs::entity_t cam = world->create_entity();
		thread::LockedValue<ecs::registry> ecs = world->ecs();
		ecs->add_component<Camera>(cam);

		Transform& trans = ecs->get_component<Transform>(cam);
		trans.position = glm::vec3(-3.0f, 1.0f, 0.0f);
		ecs->get_component<Name>(cam).name = "Main camera";

		renderer->create_viewport(100, 100, cam); // Will be resized anyway
	}

    srand(time(nullptr));
    for (int i = 0; i < 1; ++i) {
        ecs::entity_t light = world->create_entity();
        thread::LockedValue<ecs::registry> ecs = world->ecs();
        auto& info = ecs->add_component<PointLight>(light);
        info.radius = rand() % 5 + 3;
        info.intensity = rand() % 3 + 2;
        auto& pos = ecs->get_component<Transform>(light).position;
        const int concentration = 10;
        pos.x = rand() % concentration - concentration / 2.0;
        pos.y = rand() % concentration - concentration / 2.0;
        pos.z = rand() % concentration - concentration / 2.0;
        ecs->get_component<Name>(light).name = "Light" + std::to_string(i);

        info.radius = 10;
        info.intensity = 50;
        pos.x = -1;
        pos.y = 3;
        pos.z = 0;
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