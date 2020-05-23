#include <andromeda/renderer/geometry_pass.hpp>
#include <stl/literals.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/core/context.hpp>
#include <andromeda/renderer/util.hpp>

#include <phobos/core/vulkan_context.hpp>
#include <phobos/present/present_manager.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace andromeda::renderer {

GeometryPass::GeometryPass(Context& ctx, ph::PresentManager& vk_present) {
	create_pipeline(ctx);

	depth = &vk_present.add_depth_attachment("depth", {1280, 720});
	normal = &vk_present.add_color_attachment("normal", { 1280, 720 }, vk::Format::eR16G16B16A16Unorm);
	albedo_ao = &vk_present.add_color_attachment("albedo_ao", { 1280, 720 }, vk::Format::eR8G8B8A8Unorm);
	metallic_roughness = &vk_present.add_color_attachment("metallic_roughness", { 1280, 720 }, vk::Format::eR8G8Unorm);
}

void GeometryPass::build(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database) {
	// Reset per-frame buffers
	per_frame_buffers = {};

	ph::RenderPass pass;
#if ANDROMEDA_DEBUG
	pass.debug_name = "deferred_geometry_pass";
#endif
	pass.outputs = { *normal, *albedo_ao, *metallic_roughness, *depth };
	vk::ClearValue clear_color = vk::ClearColorValue{ std::array<float, 4>{ {0.0f, 0.0f, 0.0f, 1.0f}} };
	vk::ClearValue clear_depth = vk::ClearDepthStencilValue{ 1.0f, 0 };
	pass.clear_values = { clear_color, clear_color, clear_color, clear_depth };

	pass.callback = [this, &frame, &database, &ctx](ph::CommandBuffer& cmd_buf) {
		// Setup automatic viewport because we just want to cover the full output attachment
		auto_viewport_scissor(cmd_buf);

		// Note that we must set the viewport and scissor state before early returning, this is because we specified them as dynamic states.
		// This requires us to have commands to update them.
		if (database.draws.empty()) return;

		// Bind the pipeline
		ph::Pipeline pipeline = cmd_buf.get_pipeline("deferred_geometry");
		cmd_buf.bind_pipeline(pipeline);

		update_transforms(cmd_buf, database);
		update_camera_data(cmd_buf, database);

		// Bind descriptor set
		vk::DescriptorSet descr_set = get_descriptors(frame, cmd_buf, database);
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
			// Execute drawcall
			cmd_buf.draw_indexed(mesh->index_count(), 1, 0, 0, 0);
		}
	};

	graph.add_pass(std::move(pass));
}

void GeometryPass::create_pipeline(Context& ctx) {
	using namespace stl::literals;

	ph::PipelineCreateInfo pci;
	// Deferred geometry pass needs two color attachments (one for albedo + specular, one for normal), so we need two blend attachments
	vk::PipelineColorBlendAttachmentState blend;
	blend.blendEnable = false;
	blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	pci.blend_attachments.push_back(blend);
	pci.blend_attachments.push_back(blend);

	// These need a special blend state since only the R and G channels exist
	vk::PipelineColorBlendAttachmentState blend_metallic_roughness;
	blend.blendEnable = false;
	blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG;
	pci.blend_attachments.push_back(blend);

#if ANDROMEDA_DEBUG
	pci.debug_name = "deferred_geometry_pipeline";
#endif

	pci.dynamic_states.push_back(vk::DynamicState::eViewport);
	pci.dynamic_states.push_back(vk::DynamicState::eScissor);

	pci.viewports.emplace_back();
	pci.scissors.emplace_back();

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

	std::vector<uint32_t> vert_code = ph::load_shader_code("data/shaders/geometry.vert.spv");
	std::vector<uint32_t> frag_code = ph::load_shader_code("data/shaders/geometry.frag.spv");

	ph::ShaderHandle vertex_shader = ph::create_shader(*ctx.vulkan, vert_code, "main", vk::ShaderStageFlagBits::eVertex);
	ph::ShaderHandle fragment_shader = ph::create_shader(*ctx.vulkan, frag_code, "main", vk::ShaderStageFlagBits::eFragment);

	pci.shaders.push_back(vertex_shader);
	pci.shaders.push_back(fragment_shader);

	ph::reflect_shaders(*ctx.vulkan, pci);
	// Store bindings so we don't need to look them up every frame
	bindings.camera = pci.shader_info["camera"];
	bindings.transforms = pci.shader_info["transforms"];
	bindings.textures = pci.shader_info["textures"];

	ctx.vulkan->pipelines.create_named_pipeline("deferred_geometry", std::move(pci));
}

void GeometryPass::update_transforms(ph::CommandBuffer& cmd_buf, RenderDatabase& database) {
	vk::DeviceSize const size = database.transforms.size() * sizeof(glm::mat4);
	per_frame_buffers.transforms = cmd_buf.allocate_scratch_ssbo(size);
	std::memcpy(per_frame_buffers.transforms.data, database.transforms.data(), size);
}

void GeometryPass::update_camera_data(ph::CommandBuffer& cmd_buf, RenderDatabase& database) {
	// The deferred main pass needs a mat4 for the projection_view and a vec3 padded to vec4 for the camera position
	vk::DeviceSize const size = sizeof(glm::mat4) + sizeof(glm::vec4);
	per_frame_buffers.camera_data = cmd_buf.allocate_scratch_ubo(size);

	std::memcpy(per_frame_buffers.camera_data.data, glm::value_ptr(database.projection_view), sizeof(glm::mat4));
	std::memcpy(per_frame_buffers.camera_data.data + sizeof(glm::mat4), glm::value_ptr(database.camera_position), sizeof(glm::vec3));
}


vk::DescriptorSet GeometryPass::get_descriptors(ph::FrameInfo& frame, ph::CommandBuffer& cmd_buf, RenderDatabase& database) {
	ph::DescriptorSetBinding set;

	set.add(ph::make_descriptor(bindings.textures, database.texture_views, frame.default_sampler));
	set.add(ph::make_descriptor(bindings.camera, per_frame_buffers.camera_data));
	set.add(ph::make_descriptor(bindings.transforms, per_frame_buffers.transforms));

	// We need variable count to use descriptor indexing
	vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_count_info;
	uint32_t counts[1]{ ph::meta::max_unbounded_array_size };
	variable_count_info.descriptorSetCount = 1;
	variable_count_info.pDescriptorCounts = counts;

	return cmd_buf.get_descriptor(set, &variable_count_info);
}

}