#include <andromeda/wsi/application.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/util/log.hpp>
#include <andromeda/util/version.hpp>

#include <phobos/core/vulkan_context.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/assets/texture.hpp>
#include <andromeda/components/dbg_quad.hpp>

namespace andromeda::wsi {

Application::Application(size_t width, size_t height, std::string_view title)
	: window(width, height, title) {

	ph::AppSettings settings;
#if ANDROMEDA_DEBUG
	settings.enable_validation_layers = true;
#else
	settings.enabe_validation_layers = false;
#endif
	constexpr Version v = ANDROMEDA_VERSION;
	settings.version = { v.major, v.minor, v.patch };
	context.vulkan = std::unique_ptr<ph::VulkanContext>(ph::create_vulkan_context(*window.handle(), &io::get_console_logger(), settings));
	context.world = std::make_unique<world::World>();

	renderer = std::make_unique<renderer::Renderer>(context);
}

Application::~Application() {
	context.vulkan->device.waitIdle();
	renderer.reset(nullptr);
	window.close();
	context.world.reset(nullptr);
	context.vulkan.reset(nullptr);
}

void Application::run() {
	using namespace components;

	Handle<Texture> tex = assets::load<Texture>(context, "data/textures/test.png");
	ecs::entity_t ent = context.world->create_entity();
	auto& quad = context.world->ecs().add_component<DbgQuad>(ent);
	quad.texture = tex;

	while (window.is_open()) {
		window.poll_events();

		renderer->render(context);
	}
}

}