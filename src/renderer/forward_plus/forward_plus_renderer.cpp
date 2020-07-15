#include <andromeda/renderer/renderer.hpp>

#include <phobos/present/present_manager.hpp>
#include <andromeda/renderer/util.hpp>
#include <andromeda/assets/assets.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <stl/literals.hpp>

namespace andromeda::renderer {

ForwardPlusRenderer::ForwardPlusRenderer(Context& ctx) : Renderer(ctx) {
	color = &vk_present->add_color_attachment("fwd_plus_color", { 1920, 1088 }, vk::Format::eR16G16B16A16Sfloat);
	depth = &vk_present->add_depth_attachment("fwd_plus_depth", { 1920, 1088 });

	brdf_lookup = ctx.request_texture("data/textures/ibl_brdf_lut.png", false);

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
	sampler_info.maxLod = 64.0f;
	sampler_info.anisotropyEnable = true;
	sampler_info.maxAnisotropy = 16;
	sampler = ctx.vulkan->device.createSampler(sampler_info);
}

void ForwardPlusRenderer::render_frame(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph) {
	per_frame_buffers = {};
	depth_prepass(graph, ctx);
	light_cull(graph, ctx);
	shading(graph, ctx);
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
	pass.debug_name = "fwd_plus_light_cull_compute_pass";
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

		// Insert barrier to protect the visibility buffer
		vk::BufferMemoryBarrier barrier;
		barrier.buffer = per_frame_buffers.visible_light_indices.buffer;
		barrier.offset = per_frame_buffers.visible_light_indices.offset;
		barrier.size = per_frame_buffers.visible_light_indices.range;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		cmd_buf.barrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eFragmentShader, barrier);
	};

	graph.add_pass(std::move(pass));
}

void ForwardPlusRenderer::shading(ph::RenderGraph& graph, Context& ctx) {
	ph::RenderPass pass;
#if ANDROMEDA_DEBUG
	pass.debug_name = "fwd_plus_shading_pass";
#endif
	pass.outputs = { *color, *depth };
	// Note that we don't want to clear the depth attachment
	pass.clear_values = { vk::ClearColorValue{std::array<float, 4>{{0, 0, 0, 1}}} };

	pass.callback = [this, &ctx](ph::CommandBuffer& cmd_buf) {
		// Setup automatic viewport because we just want to cover the full output attachment
		auto_viewport_scissor(cmd_buf);

		// Note that we must set the viewport and scissor state before early returning, this is because we specified them as dynamic states.
		// This requires us to have commands to update them.
		if (!assets::is_ready(database.environment_map) || !assets::is_ready(brdf_lookup)) { return; }
		if (database.draws.empty()) return;

		// Bind the pipeline
		ph::Pipeline pipeline = cmd_buf.get_pipeline("fwd_plus_shading");
		cmd_buf.bind_pipeline(pipeline);

		// Bind descriptor set
		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(shading_bindings.textures, database.texture_views, sampler));
		set_binding.add(ph::make_descriptor(shading_bindings.camera, per_frame_buffers.camera));
		set_binding.add(ph::make_descriptor(shading_bindings.transforms, per_frame_buffers.transforms));
		set_binding.add(ph::make_descriptor(shading_bindings.lights, per_frame_buffers.lights));
		set_binding.add(ph::make_descriptor(shading_bindings.visible_light_indices, per_frame_buffers.visible_light_indices));
		EnvMap* env_map = assets::get(database.environment_map);
		set_binding.add(ph::make_descriptor(shading_bindings.brdf_lookup, assets::get(brdf_lookup)->get_view(), sampler));
		set_binding.add(ph::make_descriptor(shading_bindings.irradiance_map, env_map->irradiance_map_view, sampler));
		set_binding.add(ph::make_descriptor(shading_bindings.specular_map, env_map->specular_map_view, sampler));
		// We need variable count to use descriptor indexing
		vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_count_info;
		uint32_t counts[1]{ ph::meta::max_unbounded_array_size };
		variable_count_info.descriptorSetCount = 1;
		variable_count_info.pDescriptorCounts = counts;
		vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding, &variable_count_info);
		cmd_buf.bind_descriptor_set(0, descr_set);

		for (uint32_t draw_idx = 0; draw_idx < database.draws.size(); ++draw_idx) {
			auto const& draw = database.draws[draw_idx];

			// Skip this draw if the textures for it aren't loaded yet
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
			auto indices = database.get_material_textures(draw.material);
			uint32_t texture_indices[]{ indices.color, indices.normal, indices.metallic, indices.roughness, indices.ambient_occlusion };
			cmd_buf.push_constants(vk::ShaderStageFlagBits::eFragment, sizeof(uint32_t), sizeof(texture_indices), &texture_indices);
			uint32_t tile_count_x = 1920 / 16; // TODO: don't hardcode
			cmd_buf.push_constants(vk::ShaderStageFlagBits::eFragment, 6 * sizeof(uint32_t), sizeof(uint32_t), &tile_count_x);
			// Execute drawcall
			cmd_buf.draw_indexed(mesh->index_count(), 1, 0, 0, 0);
		}
	};

	graph.add_pass(std::move(pass));
}

void ForwardPlusRenderer::update_transforms(ph::CommandBuffer& cmd_buf) {
	vk::DeviceSize const size = database.transforms.size() * sizeof(glm::mat4);
	per_frame_buffers.transforms = cmd_buf.allocate_scratch_ssbo(size);
	std::memcpy(per_frame_buffers.transforms.data, database.transforms.data(), size);
}

void ForwardPlusRenderer::update_camera_data(ph::CommandBuffer& cmd_buf) {
	vk::DeviceSize const size = 3 * sizeof(glm::mat4) + sizeof(glm::vec4);
	per_frame_buffers.camera = cmd_buf.allocate_scratch_ubo(size);

	std::byte* base_address = per_frame_buffers.camera.data;
	std::memcpy(base_address, glm::value_ptr(database.projection), sizeof(glm::mat4));
	std::memcpy(base_address + sizeof(glm::mat4), glm::value_ptr(database.view), sizeof(glm::mat4));
	std::memcpy(base_address + 2 * sizeof(glm::mat4), glm::value_ptr(database.projection_view), sizeof(glm::mat4));
	std::memcpy(base_address + 3 * sizeof(glm::mat4), glm::value_ptr(database.camera_position), sizeof(glm::vec3));
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

	// Shading
	{
		ph::PipelineCreateInfo pci;
#if ANDROMEDA_DEBUG
		pci.debug_name = "fwd_plus_depth_pipeline";
#endif
		pci.blend_logic_op_enable = false;
		vk::PipelineColorBlendAttachmentState blend;
		blend.blendEnable = false;
		blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		pci.blend_attachments.push_back(blend);

		pci.dynamic_states.push_back(vk::DynamicState::eViewport);
		pci.dynamic_states.push_back(vk::DynamicState::eScissor);
		pci.viewports.emplace_back();
		pci.scissors.emplace_back();

		// Pos + Normal + Tangent + TexCoords. 
		constexpr size_t stride = (3 + 3 + 3 + 2) * sizeof(float);
		pci.vertex_input_bindings.push_back(vk::VertexInputBindingDescription(0, stride, vk::VertexInputRate::eVertex));
		// vec3 iPos;
		pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32B32Sfloat, 0_u32);
		// vec3 iNormal
		pci.vertex_attributes.emplace_back(1_u32, 0_u32, vk::Format::eR32G32B32Sfloat, 3 * (uint32_t)sizeof(float));
		// vec3 iTangent;
		pci.vertex_attributes.emplace_back(2_u32, 0_u32, vk::Format::eR32G32B32Sfloat, 6 * (uint32_t)sizeof(float));
		// vec2 iTexCoords
		pci.vertex_attributes.emplace_back(3_u32, 0_u32, vk::Format::eR32G32Sfloat, 9 * (uint32_t)sizeof(float));

		std::vector<uint32_t> vert_code = ph::load_shader_code("data/shaders/fwd_plus_shading.vert.spv");
		std::vector<uint32_t> frag_code = ph::load_shader_code("data/shaders/fwd_plus_shading.frag.spv");
		ph::ShaderHandle vertex_shader = ph::create_shader(*ctx.vulkan, vert_code, "main", vk::ShaderStageFlagBits::eVertex);
		ph::ShaderHandle fragment_shader = ph::create_shader(*ctx.vulkan, frag_code, "main", vk::ShaderStageFlagBits::eFragment);
		pci.shaders.push_back(vertex_shader);
		pci.shaders.push_back(fragment_shader);

		// Set depth stencil state accordingly to use depth values from prepass
		pci.depth_stencil.depthTestEnable = true;
		pci.depth_stencil.depthWriteEnable = false;
		pci.depth_stencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
	
		ph::reflect_shaders(*ctx.vulkan, pci);
		// Store bindings
		shading_bindings.camera = pci.shader_info["camera"];
		shading_bindings.transforms = pci.shader_info["transforms"];
		shading_bindings.textures = pci.shader_info["textures"];
		shading_bindings.lights = pci.shader_info["lights"];
		shading_bindings.visible_light_indices = pci.shader_info["visible_light_indices"];
		shading_bindings.brdf_lookup = pci.shader_info["brdf_lookup"];
		shading_bindings.irradiance_map = pci.shader_info["irradiance_map"];
		shading_bindings.specular_map = pci.shader_info["specular_map"];

		ctx.vulkan->pipelines.create_named_pipeline("fwd_plus_shading", std::move(pci));
	}
}

}