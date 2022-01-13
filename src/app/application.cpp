#include <andromeda/app/application.hpp>

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

//    Handle<ecs::entity_t> horse = assets::load<ecs::entity_t>("data/horse/horse_statue_01_4k.ent");
//    world->import_entity(*assets::get(horse));

//    Handle<ecs::entity_t> sponza = assets::load<ecs::entity_t>("data/sponza/Sponza.ent");
//    world->import_entity(*assets::get(sponza));

    Handle<ecs::entity_t> cart = assets::load<ecs::entity_t>("data/coffeecart/CoffeeCart_01_2k.ent");
    world->import_entity(*assets::get(cart));

    Handle<ecs::entity_t> ground = assets::load<ecs::entity_t>("data/scene/geometry.ent");
    world->import_entity(*assets::get(ground));

//    Handle<ecs::entity_t> gallery = assets::load<ecs::entity_t>("data/gallery/gallery.ent");
//    world->import_entity(*assets::get(gallery));

//    Handle<ecs::entity_t> rungholt = assets::load<ecs::entity_t>("data/rungholt/rungholt.ent");
//    world->import_entity(*assets::get(rungholt));

//    ecs::entity_t powerplant_root = world->create_entity();

    for (uint32_t i = 0; i < 21; ++i) {
//        Handle<ecs::entity_t> part = assets::load<ecs::entity_t>(fmt::format(FMT_STRING("data/SM_Powerplant/SM_Powerplant{}.ent"), i));
//       world->import_entity(*assets::get(part), powerplant_root);
    }

    uint64_t frame = 0;
    while (window->is_open()) {
        window->poll_events();
        gfx::imgui::new_frame();

        editor->update(*world, *graphics, *renderer);
        renderer->render_frame(*graphics, *world);

        ++frame;
        // Flush every 10 frames
        if (frame % 10) { log->flush(); }
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