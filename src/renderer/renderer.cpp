#include <andromeda/renderer/renderer.hpp>

#include <phobos/renderer/renderer.hpp>
#include <phobos/present/present_manager.hpp>
#include <phobos/core/vulkan_context.hpp>
#include <phobos/pipeline/shader_info.hpp>
#include <stl/literals.hpp>

#include <andromeda/util/version.hpp>
#include <andromeda/util/log.hpp>

#include <andromeda/world/world.hpp>

#include <andromeda/components/dbg_quad.hpp>

namespace andromeda::renderer {

Renderer::Renderer(Context& ctx) {
	using namespace stl::literals;

	vk_present = std::make_unique<ph::PresentManager>(*ctx.vulkan);
	vk_renderer = std::make_unique<ph::Renderer>(*ctx.vulkan);

	// Create fullscreen quad pipeline (for debugging, but might be useful later)s
	ph::PipelineCreateInfo pci;
	vk::PipelineColorBlendAttachmentState blend;
	blend.blendEnable = false;
	blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	pci.blend_logic_op_enable = false;
#if ANDROMEDA_DEBUG
	pci.debug_name = "fullscreen_pipeline";
#endif
	pci.dynamic_states.push_back(vk::DynamicState::eViewport);
	pci.dynamic_states.push_back(vk::DynamicState::eScissor);
	
	pci.viewports.emplace_back();
	pci.scissors.emplace_back();

	ph::ShaderHandle vert = ph::create_shader(*ctx.vulkan, ph::load_shader_code("data/shaders/fullscreen.vert.spv"), "main", vk::ShaderStageFlagBits::eVertex);
	ph::ShaderHandle frag = ph::create_shader(*ctx.vulkan, ph::load_shader_code("data/shaders/fullscreen.frag.spv"), "main", vk::ShaderStageFlagBits::eFragment);

	pci.shaders.push_back(vert);
	pci.shaders.push_back(frag);

	pci.vertex_input_binding.binding = 0;
	pci.vertex_input_binding.stride = 4 * sizeof(float);
	pci.vertex_input_binding.inputRate = vk::VertexInputRate::eVertex;

	pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32Sfloat, 0_u32);
	pci.vertex_attributes.emplace_back(1_u32, 0_u32, vk::Format::eR32G32Sfloat, (uint32_t)(2 * sizeof(float)));

	ph::reflect_shaders(*ctx.vulkan, pci);

	ctx.vulkan->pipelines.create_named_pipeline("fullscreen", std::move(pci));
}

Renderer::~Renderer() {
	vk_renderer->destroy();
	vk_present->destroy();
}

void Renderer::render(Context& ctx) {
	using namespace components;

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
	present_pass.callback = [](ph::CommandBuffer& cmd_buf) {
		
	};

	graph.add_pass(std::move(present_pass));

	graph.build();

	frame.render_graph = &graph;
	vk_renderer->render_frame(frame);
	vk_present->present_frame(frame);
}

}