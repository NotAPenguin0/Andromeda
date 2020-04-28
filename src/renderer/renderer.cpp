#include <andromeda/renderer/renderer.hpp>

#include <phobos/renderer/renderer.hpp>
#include <phobos/present/present_manager.hpp>
#include <phobos/core/vulkan_context.hpp>

#include <andromeda/util/version.hpp>
#include <andromeda/util/log.hpp>

namespace andromeda::renderer {

Renderer::Renderer(wsi::Window& window) {
	ph::AppSettings settings;
#if ANDROMEDA_DEBUG
	settings.enable_validation_layers = true;
#else
	settings.enabe_validation_layers = false;
#endif
	constexpr Version v = ANDROMEDA_VERSION;
	settings.version = { v.major, v.minor, v.patch };
	vk_context = ph::create_vulkan_context(*window.handle(), &io::get_console_logger(), settings);

	vk_present = std::make_unique<ph::PresentManager>(*vk_context);
	vk_renderer = std::make_unique<ph::Renderer>(*vk_context);
}

Renderer::~Renderer() {
	vk_renderer->destroy();
	vk_present->destroy();
	ph::destroy_vulkan_context(vk_context);
}

}