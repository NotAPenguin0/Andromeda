#include <andromeda/renderer/renderer.hpp>

#include <phobos/renderer/renderer.hpp>
#include <phobos/present/present_manager.hpp>
#include <phobos/core/vulkan_context.hpp>

#include <andromeda/util/version.hpp>
#include <andromeda/util/log.hpp>

#include <andromeda/world/world.hpp>

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
	vk_context->device.waitIdle();
	vk_renderer->destroy();
	vk_present->destroy();
	ph::destroy_vulkan_context(vk_context);
}

void Renderer::render(world::World const& world) {
	vk_present->wait_for_available_frame();

	ph::FrameInfo& frame = vk_present->get_frame_info();
	ph::RenderGraph graph{ vk_context };

	// This renderpass is temporary, we will improve on this system when adding ImGui

	ph::RenderPass present_pass;
#if ANDROMEDA_DEBUG
	present_pass.debug_name = "present_pass";
#endif
	present_pass.clear_values = { vk::ClearColorValue{ std::array<float, 4>{ {0.0f, 0.0f, 0.0f, 1.0f}} } };
	
	ph::RenderAttachment swapchain = vk_present->get_swapchain_attachment(frame);
	present_pass.outputs = { swapchain };
	present_pass.callback = [](ph::CommandBuffer& cmd_buf) {};
	graph.add_pass(std::move(present_pass));

	graph.build();

	frame.render_graph = &graph;
	vk_renderer->render_frame(frame);
	vk_present->present_frame(frame);
}

}