#include <andromeda/renderer/tonemap_pass.hpp>

#include <andromeda/renderer/util.hpp>
#include <phobos/core/vulkan_context.hpp>

#include <stl/literals.hpp>

namespace andromeda::renderer {

static constexpr float quad_geometry[] = {
	-1, 1, 0, 1, -1, -1, 0, 0,
	1, -1, 1, 0, -1, 1, 0, 1,
	1, -1, 1, 0, 1, 1, 1, 1
};

TonemapPass::TonemapPass(Context& ctx) {
	create_pipeline(ctx);
}

void TonemapPass::build(Context& ctx, Attachments attachments, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database) {
	ph::RenderPass pass;
#if ANDROMEDA_DEBUG
	pass.debug_name = "tonemap_pass";
#endif
	pass.outputs = { attachments.output_ldr };
	pass.sampled_attachments = { attachments.input_hdr };
	pass.clear_values = { vk::ClearColorValue{ std::array<float, 4>{ {0.0f, 0.0f, 0.0f, 1.0f}} } };
	pass.callback = [this, &ctx, &frame, &database, attachments](ph::CommandBuffer& cmd_buf) {
		auto_viewport_scissor(cmd_buf);

		ph::Pipeline pipeline = cmd_buf.get_pipeline("tonemap");
		cmd_buf.bind_pipeline(pipeline);

		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(bindings.input_hdr, attachments.input_hdr.image_view(), frame.default_sampler));
		vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
		cmd_buf.bind_descriptor_set(0, descr_set);

		ph::BufferSlice quad_vbo = cmd_buf.allocate_scratch_vbo(sizeof(quad_geometry));
		std::memcpy(quad_vbo.data, quad_geometry, sizeof(quad_geometry));
		cmd_buf.bind_vertex_buffer(0, quad_vbo);

		cmd_buf.draw(6, 1, 0, 0);
	};

	graph.add_pass(std::move(pass));
}

void TonemapPass::create_pipeline(Context& ctx) {
    using namespace stl::literals;

    ph::PipelineCreateInfo pci;
    // To avoid having to recreate the pipeline when the viewport is changed
    pci.dynamic_states.push_back(vk::DynamicState::eViewport);
    pci.dynamic_states.push_back(vk::DynamicState::eScissor);
    // Even though they are dynamic states, viewportCount must be 1 if the multiple viewports feature is  not enabled. The pViewports field
    // is ignored though, so the actual values don't matter. 
    // See  also https://renderdoc.org/vkspec_chunked/chap25.html#VkPipelineViewportStateCreateInfo
    pci.viewports.emplace_back();
    pci.scissors.emplace_back();

    vk::PipelineColorBlendAttachmentState blend;
    blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    blend.blendEnable = false;
    pci.blend_attachments.push_back(blend);
#if ANDROMEDA_DEBUG
    pci.debug_name = "tonemap_pipeline";
#endif
    constexpr size_t stride = 4 * sizeof(float);
    pci.vertex_input_bindings.push_back(vk::VertexInputBindingDescription(0, stride, vk::VertexInputRate::eVertex));
    pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32Sfloat, 0_u32);
    pci.vertex_attributes.emplace_back(1_u32, 0_u32, vk::Format::eR32G32Sfloat, 2 * (uint32_t)sizeof(float));

    pci.depth_stencil.depthTestEnable = false;
    pci.depth_stencil.depthWriteEnable = false;
    // The ambient pipeline draws a fullscreen quad so we don't want any culling
    pci.rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    std::vector<uint32_t> vert_code = ph::load_shader_code("data/shaders/tonemap.vert.spv");
    std::vector<uint32_t> frag_code = ph::load_shader_code("data/shaders/tonemap.frag.spv");

    ph::ShaderHandle vertex_shader = ph::create_shader(*ctx.vulkan, vert_code, "main", vk::ShaderStageFlagBits::eVertex);
    ph::ShaderHandle fragment_shader = ph::create_shader(*ctx.vulkan, frag_code, "main", vk::ShaderStageFlagBits::eFragment);

    pci.shaders.push_back(vertex_shader);
    pci.shaders.push_back(fragment_shader);

    ph::reflect_shaders(*ctx.vulkan, pci);
    // Store bindings so we don't need to look them up every frame
    bindings.input_hdr = pci.shader_info["input_hdr"];

    ctx.vulkan->pipelines.create_named_pipeline("tonemap", std::move(pci));
}

}