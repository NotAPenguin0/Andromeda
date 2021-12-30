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

    assets::impl::set_global_pointers(graphics.get(), world.get());

	ImGui::CreateContext();
	
	editor = std::make_unique<editor::Editor>(*graphics, *window);
	renderer = std::make_unique<gfx::Renderer>(*graphics, *window);
}

int Application::run() {
    Handle<ecs::entity_t> lights_bp = assets::load<ecs::entity_t>("data/scene/lights.ent");
    Handle<ecs::entity_t> camera_bp = assets::load<ecs::entity_t>("data/scene/camera.ent");

    world->import_entity(*assets::get(lights_bp));
    ecs::entity_t camera = world->import_entity(*assets::get(camera_bp));
    renderer->create_viewport(1, 1, camera);

    Handle<ecs::entity_t> sponza = assets::load<ecs::entity_t>("data/sponza/Sponza.ent");
    world->import_entity(*assets::get(sponza));

    Handle<ecs::entity_t> pbr_spheres = assets::load<ecs::entity_t>("data/spheres/MetalRoughSpheres.ent");
    world->import_entity(*assets::get(pbr_spheres));

    {
        srand(time(nullptr));
        auto ecs = world->ecs();
        for (int i = 0; i < 100; ++i) {
            auto l = world->create_entity(ecs);
            const int concentration = 10;
            auto &pos = ecs->get_component<Transform>(l).position;
            pos.x = rand() % (concentration * 2) - concentration;
            pos.y = rand() % concentration;
            pos.z = rand() % (concentration * 2) - concentration;

            auto& light = ecs->add_component<PointLight>(l);
            light.intensity = 10;
            light.radius = rand() % (2 * concentration);
            light.color = glm::vec3(rand() % 255 / 255.0, rand() % 255 / 255.0, rand() % 255 / 255.0);
        }
    }

    uint64_t frame = 0;
	while (window->is_open()) {
		window->poll_events();
		gfx::imgui::new_frame();

		editor->update(*world, *graphics, *renderer);
		renderer->render_frame(*graphics, *world);

        ++frame;
        // Flush every 10 frames
        if (frame % 10) log->flush();
	}

	// We don't want the app to look like it's hanging while all resources are being freed, so 
	// hide the window.
	window->hide();

	graphics->wait_idle();

	assets::unload_all<gfx::Texture>();
	assets::unload_all<gfx::Mesh>();
    assets::unload_all<gfx::Material>();
    assets::unload_all<gfx::Environment>();

	scheduler->shutdown();
	graphics->wait_idle();

	renderer->shutdown(*graphics);

	return 0;
}

} // namespace andromeda