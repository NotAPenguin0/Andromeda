#include <andromeda/renderer/renderer.hpp>

#include <phobos/present/present_manager.hpp>
#include <andromeda/renderer/util.hpp>
#include <andromeda/assets/assets.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <stl/literals.hpp>

namespace andromeda::renderer {

ForwardPlusRenderer::ForwardPlusRenderer(Context& ctx) : Renderer(ctx) {
	depth = &vk_present->add_depth_attachment("fwd_plus_depth", { 1920, 1088 });

	create_pipelines(ctx);

	// Create depth sampler
	vk::SamplerCreateInfo sampler_info;
	sampler_info.magFilter = vk::Filter::eLinear;
	sampler_info.minFilter = vk::Filter::eLinear;
	sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler_info.anisotropyEnable = false;
	sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
	sampler_info.unnormalizedCoordinates = false;
	sampler_info.compareEnable = false;
	sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler_info.minLod = 0.0;
	sampler_info.maxLod = 0.0;
	sampler_info.mipLodBias = 0.0;
	depth_sampler = ctx.vulkan->device.createSampler(sampler_info);
}

void ForwardPlusRenderer::render_frame(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph) {
	per_frame_buffers = {};
	depth_prepass(graph, ctx);
	light_cull(graph, ctx);
}

void ForwardPlusRenderer::depth_prepass(ph::RenderGraph& graph, Context& ctx) {
	ph::RenderPass pass;
#if ANDROMEDA_DEBUG
	pass.debug_name = "fwd_plus_depth_prepass";
#endif
	pass.clear_values = { vk::ClearDepthStencilValue{1.0, 0} };
	pass.outputs = { *depth };
	pass.callback = [this, &ctx](ph::CommandBuffer& cmd_buf) {
		auto_viewport_scissor(cmd_buf);

		if (database.draws.empty()) { return; }

		update_camera_data(cmd_buf);
		update_transforms(cmd_buf);

		ph::Pipeline pipeline = cmd_buf.get_pipeline("fwd_plus_depth");
		cmd_buf.bind_pipeline(pipeline);

		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(depth_bindings.camera, per_frame_buffers.camera));
		set_binding.add(ph::make_descriptor(depth_bindings.transforms, per_frame_buffers.transforms));
		vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
		cmd_buf.bind_descriptor_set(0, descr_set);

		for (uint32_t draw_idx = 0; draw_idx < database.draws.size(); ++draw_idx) {
			auto const& draw = database.draws[draw_idx];

			// Skip this draw if the textures for it aren't loaded yet (even though the textures aren't used).
			// We skip anyway because we can't process these draws furhter down the pipeline anyway.
			Material* material = assets::get(draw.material);
			if (!assets::is_ready(material->color) || !assets::is_ready(material->normal) || !assets::is_ready(material->metallic)
				|| !assets::is_ready(material->roughness) || !assets::is_ready(material->ambient_occlusion)) {
				continue;
			}

			// Don't draw if the mesh isn't ready
			if (!assets::is_ready(draw.mesh)) {
				continue;
			}

			Mesh* mesh = assets::get(draw.mesh);
			// Bind draw data
			cmd_buf.bind_vertex_buffer(0, ph::whole_buffer_slice(*ctx.vulkan, mesh->get_vertices()));
			cmd_buf.bind_index_buffer(ph::whole_buffer_slice(*ctx.vulkan, mesh->get_indices()));

			// update push constant ranges
			stl::uint32_t const transform_index = draw_idx;
			cmd_buf.push_constants(vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &transform_index);
			
			// Execute drawcall
			cmd_buf.draw_indexed(mesh->index_count(), 1, 0, 0, 0);
		}
	};

	graph.add_pass(std::move(pass));
}

void ForwardPlusRenderer::light_cull(ph::RenderGraph& graph, Context& ctx) {
	ph::RenderPass pass;
#if ANDROMEDA_DEBUG
	pass.debug_name = "fwd_plus_light_cull_compute";
#endif
	pass.sampled_attachments = { *depth };
	pass.outputs = {}; // No outputs, this makes this pass a compute pass
	pass.callback = [this](ph::CommandBuffer& cmd_buf) {

		if (database.draws.empty()) { return; }

		ph::Pipeline pipeline = cmd_buf.get_compute_pipeline("fwd_plus_light_cull");
		cmd_buf.bind_pipeline(pipeline);

		update_lights(cmd_buf);
		create_compute_output_buffer(cmd_buf);

		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(compute_bindings.depth, depth->image_view(), depth_sampler));
		set_binding.add(ph::make_descriptor(compute_bindings.camera, per_frame_buffers.camera));
		set_binding.add(ph::make_descriptor(compute_bindings.lights, per_frame_buffers.lights));
		set_binding.add(ph::make_descriptor(compute_bindings.visible_lights_output, per_frame_buffers.visible_light_indices));
		vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
		cmd_buf.bind_descriptor_set(0, descr_set);

		//TODO: Don't hardcode this
		uint32_t screen_size[2]{ 1920, 1088 };
		uint32_t const tile_count_x = (screen_size[0] / 16);
		uint32_t const tile_count_y = (screen_size[1] / 16); 
		cmd_buf.push_constants(vk::ShaderStageFlagBits::eCompute, 0, 2 * sizeof(uint32_t), &screen_size);
		cmd_buf.dispatch_compute(tile_count_x, tile_count_y, 1);
	};

	graph.add_pass(std::move(pass));
}

void ForwardPlusRenderer::update_transforms(ph::CommandBuffer& cmd_buf) {
	vk::DeviceSize const size = database.transforms.size() * sizeof(glm::mat4);
	per_frame_buffers.transforms = cmd_buf.allocate_scratch_ssbo(size);
	std::memcpy(per_frame_buffers.transforms.data, database.transforms.data(), size);
}

void ForwardPlusRenderer::update_camera_data(ph::CommandBuffer& cmd_buf) {
	vk::DeviceSize const size = 3 * sizeof(glm::mat4);
	per_frame_buffers.camera = cmd_buf.allocate_scratch_ubo(size);

	std::byte* base_address = per_frame_buffers.camera.data;
	std::memcpy(base_address, glm::value_ptr(database.projection), sizeof(glm::mat4));
	std::memcpy(base_address + sizeof(glm::mat4), glm::value_ptr(database.view), sizeof(glm::mat4));
	std::memcpy(base_address + 2 * sizeof(glm::mat4), glm::value_ptr(database.projection_view), sizeof(glm::mat4));
}

void ForwardPlusRenderer::update_lights(ph::CommandBuffer& cmd_buf) {
	vk::DeviceSize const size = database.point_lights.size() * sizeof(RenderDatabase::InternalPointLight);
	per_frame_buffers.lights = cmd_buf.allocate_scratch_ssbo(size);
	
	std::memcpy(per_frame_buffers.lights.data, database.point_lights.data(), size);
}

void ForwardPlusRenderer::create_compute_output_buffer(ph::CommandBuffer& cmd_buf) {
	uint32_t const tile_count = (1920 / 16) * (1088 / 16); //TODO: Don't hardcode this
	uint32_t const max_lights_per_tile = 1024;
	vk::DeviceSize size = tile_count * max_lights_per_tile * sizeof(int32_t);
	per_frame_buffers.visible_light_indices = cmd_buf.allocate_scratch_ssbo(size);
}

void ForwardPlusRenderer::create_pipelines(Context& ctx) {
	using namespace stl::literals;

	// Depth pre-pass
	{
		ph::PipelineCreateInfo pci;
#if ANDROMEDA_DEBUG
		pci.debug_name = "fwd_plus_depth_pipeline";
#endif
		pci.blend_logic_op_enable = false;
		pci.blend_attachments = {};

		pci.dynamic_states.push_back(vk::DynamicState::eViewport);
		pci.dynamic_states.push_back(vk::DynamicState::eScissor);
		pci.viewports.emplace_back();
		pci.scissors.emplace_back();

		// Pos + Normal + Tangent + TexCoords. Note that we don't specify attributes for the last 3, because they are unused.
		// Instead, we just specify an appropriate stride to make sure buffer access is correct
		constexpr size_t stride = (3 + 3 + 3 + 2) * sizeof(float);
		pci.vertex_input_bindings.push_back(vk::VertexInputBindingDescription(0, stride, vk::VertexInputRate::eVertex));
		// vec3 iPos;
		pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32B32Sfloat, 0_u32);

		// We don't need a fragment shader for a depth-only pass.
		std::vector<uint32_t> vert_code = ph::load_shader_code("data/shaders/fwd_plus_depth.vert.spv");
		ph::ShaderHandle vertex_shader = ph::create_shader(*ctx.vulkan, vert_code, "main", vk::ShaderStageFlagBits::eVertex);
		pci.shaders.push_back(vertex_shader);

		ph::reflect_shaders(*ctx.vulkan, pci);
		// Store bindings so we don't need to look them up every frame
		depth_bindings.camera = pci.shader_info["camera"];
		depth_bindings.transforms = pci.shader_info["transforms"];

		ctx.vulkan->pipelines.create_named_pipeline("fwd_plus_depth", std::move(pci));
	}
	
	// Light culling (compute)
	{
		ph::ComputePipelineCreateInfo pci;
#if ANDROMEDA_DEBUG
		pci.debug_name = "fwd_plus_light_cull_pipeline";
#endif
		std::vector<uint32_t> shader_code = ph::load_shader_code("data/shaders/fwd_plus_light_cull.comp.spv");
		ph::ShaderHandle shader = ph::create_shader(*ctx.vulkan, shader_code, "main", vk::ShaderStageFlagBits::eCompute);
		pci.shader = shader;

		ph::reflect_shaders(*ctx.vulkan, pci);
		// Store bindings
		compute_bindings.camera = pci.shader_info["camera"];
		compute_bindings.depth = pci.shader_info["depth_map"];
		compute_bindings.lights = pci.shader_info["lights"];
		compute_bindings.visible_lights_output = pci.shader_info["visible_lights_out"];

		ctx.vulkan->pipelines.create_named_pipeline("fwd_plus_light_cull", std::move(pci));
	}
}

}