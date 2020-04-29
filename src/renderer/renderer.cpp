#include <andromeda/renderer/renderer.hpp>

#include <phobos/renderer/renderer.hpp>
#include <phobos/present/present_manager.hpp>
#include <phobos/core/vulkan_context.hpp>

#include <andromeda/util/version.hpp>
#include <andromeda/util/log.hpp>

#include <andromeda/world/world.hpp>

namespace andromeda::renderer {

Renderer::Renderer(Context& ctx) {
	vk_present = std::make_unique<ph::PresentManager>(*ctx.vulkan);
	vk_renderer = std::make_unique<ph::Renderer>(*ctx.vulkan);
}

Renderer::~Renderer() {
	vk_renderer->destroy();
	vk_present->destroy();
}

void Renderer::render(Context& ctx) {
	vk_present->wait_for_available_frame();

	ph::FrameInfo& frame = vk_present->get_frame_info();
	ph::RenderGraph graph{ ctx.vulkan.get() };

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