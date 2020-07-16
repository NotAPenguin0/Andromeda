#include <andromeda/renderer/deferred/skybox_pass.hpp>


#include <andromeda/core/context.hpp>
#include <andromeda/renderer/util.hpp>
#include <andromeda/assets/assets.hpp>

#include <phobos/core/vulkan_context.hpp>
#include <phobos/present/present_manager.hpp>

#include <stl/literals.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace andromeda::renderer::deferred {

static constexpr float skybox_vertices[] = {
	// back
	-1, -1, -1, 1, 1, -1,
	1, -1, -1, 1, 1, -1,
	-1, -1, -1, -1, 1, -1,
	// front 
	-1, -1, 1, 1, -1, 1,
	1, 1, 1, 1, 1, 1,
	-1, 1, 1, -1, -1, 1,
	// left 
	-1, 1, -1, -1, -1, -1,
	-1, 1, 1, -1, -1, -1,
	-1, -1, 1,-1, 1, 1,
	// right 
	1, 1, 1, 1, -1, -1,
	1, 1, -1,1, -1, -1,
	1, 1, 1, 1, -1, 1,
	// bottom 
	-1, -1, -1, 1, -1, -1,
	1, -1, 1, 1, -1, 1, 
	-1, -1, 1, -1, -1, -1,
	// top 
	-1, 1, -1, 1, 1, 1,
	1, 1, -1, 1, 1, 1,
	-1, 1, -1, -1, 1, 1
};

SkyboxPass::SkyboxPass(Context& ctx, ph::PresentManager& vk_present) {
	create_pipeline(ctx);
}

void SkyboxPass::build(Context& ctx, Attachments attachments, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database) {
	if (database.environment_map.id == Handle<EnvMap>::none || !assets::is_ready(database.environment_map)) {
		return;
	}

	ph::RenderPass pass;
#if ANDROMEDA_DEBUG
	pass.debug_name = "skybox_pass";
#endif
	pass.outputs = { attachments.output, attachments.depth };

	pass.callback = [this, &frame, &database, attachments](ph::CommandBuffer& cmd_buf) {
		auto_viewport_scissor(cmd_buf);
		ph::BufferSlice ubo = cmd_buf.allocate_scratch_ubo(2 * sizeof(glm::mat4));
		std::memcpy(ubo.data, glm::value_ptr(database.projection), sizeof(glm::mat4));
		std::memcpy(ubo.data + sizeof(glm::mat4), glm::value_ptr(database.view), sizeof(glm::mat4));

		ph::BufferSlice cube_mesh = cmd_buf.allocate_scratch_vbo(sizeof(skybox_vertices));
		std::memcpy(cube_mesh.data, skybox_vertices, sizeof(skybox_vertices));

		ph::Pipeline pipeline = cmd_buf.get_pipeline("skybox");
		cmd_buf.bind_pipeline(pipeline);

		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(bindings.transform, ubo));
		EnvMap* env_map = assets::get(database.environment_map);
		set_binding.add(ph::make_descriptor(bindings.skybox, env_map->cube_map_view, frame.default_sampler));
		vk::DescriptorSet set = cmd_buf.get_descriptor(set_binding);

		cmd_buf.bind_descriptor_set(0, set);
		cmd_buf.bind_vertex_buffer(0, cube_mesh);
		cmd_buf.draw(36, 1, 0, 0);
	};

	graph.add_pass(std::move(pass));
}

void SkyboxPass::create_pipeline(Context& ctx) {
	using namespace stl::literals;

	ph::PipelineCreateInfo pci;

	vk::PipelineColorBlendAttachmentState blend_attachment;
	blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	blend_attachment.blendEnable = false;
	pci.blend_attachments.push_back(blend_attachment);

	pci.vertex_input_bindings.push_back(vk::VertexInputBindingDescription(0, 3 * sizeof(float), vk::VertexInputRate::eVertex));
	pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32B32Sfloat, 0_u32);

	pci.dynamic_states.push_back(vk::DynamicState::eViewport);
	pci.dynamic_states.push_back(vk::DynamicState::eScissor);

	// Note that these are dynamic state so we don't need to fill in the fields
	pci.viewports.emplace_back();
	pci.scissors.emplace_back();

	pci.rasterizer.cullMode = vk::CullModeFlagBits::eNone;
	pci.depth_stencil.depthTestEnable = true;
	pci.depth_stencil.depthWriteEnable = true;
	pci.depth_stencil.depthBoundsTestEnable = false;
	// We want to render the skybox if the depth value is 1.0 (which it is), so we need to set LessOrEqual
	pci.depth_stencil.depthCompareOp = vk::CompareOp::eLessOrEqual;

	pci.shaders.push_back(ph::create_shader(*ctx.vulkan, ph::load_shader_code("data/shaders/skybox.vert.spv"),
		"main", vk::ShaderStageFlagBits::eVertex));
	pci.shaders.push_back(ph::create_shader(*ctx.vulkan, ph::load_shader_code("data/shaders/skybox.frag.spv"),
		"main", vk::ShaderStageFlagBits::eFragment));

	ph::reflect_shaders(*ctx.vulkan, pci);
	bindings.transform = pci.shader_info["transform"];
	bindings.skybox = pci.shader_info["skybox"];

	ctx.vulkan->pipelines.create_named_pipeline("skybox", stl::move(pci));
}

}