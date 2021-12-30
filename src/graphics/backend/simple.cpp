#include <andromeda/graphics/backend/simple.hpp>


using namespace std::literals::string_literals;

namespace andromeda::gfx::backend {

SimpleRenderer::SimpleRenderer(gfx::Context& ctx) : RendererBackend(ctx) {
	// Create depth attachments
	depth_attachments.resize(gfx::MAX_VIEWPORTS);
	for (uint32_t i = 0; i < gfx::MAX_VIEWPORTS; ++i) {
		depth_attachments[i] = gfx::Viewport::local_string(i, "simple_renderer_depth");
		// Create with size 1, 1 for now, we'll resize according to viewport width/height after.
		ctx.create_attachment(depth_attachments[i], VkExtent2D{ 1, 1 }, VK_FORMAT_D32_SFLOAT, ph::ImageType::DepthStencilAttachment);
	}

	// Create simple rendering pipeline
	ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "simple_pipeline")
		.add_shader("data/shaders/simple.vert.spv", "main", ph::ShaderStage::Vertex)
		.add_shader("data/shaders/simple.frag.spv", "main", ph::ShaderStage::Fragment)
		.add_vertex_input(0)
		.add_vertex_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT) // iPos
		.add_vertex_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT) // iNormal
		.add_vertex_attribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT) // iTangent
		.add_vertex_attribute(0, 3, VK_FORMAT_R32G32_SFLOAT) // iUV
		.add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
		.add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
		.add_blend_attachment()
		.set_depth_test(true)
		.set_depth_write(true)
		.set_depth_op(VK_COMPARE_OP_LESS_OR_EQUAL)
		.set_cull_mode(VK_CULL_MODE_BACK_BIT)
		.reflect()
		.get();
	ctx.create_named_pipeline(std::move(pci));
}

void SimpleRenderer::render_scene(ph::RenderGraph& graph, ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
	std::string_view color = viewport.target();
	std::string_view depth = depth_attachments[viewport.index()];

	ph::PassBuilder builder = ph::PassBuilder::create(gfx::Viewport::local_string(viewport, "simple_renderer_pass"));
	builder.add_attachment(color, ph::LoadOp::Clear, ph::ClearValue{ .color = {0.0f, 0.0f, 0.0f, 1.0f} });
	builder.add_depth_attachment(depth, ph::LoadOp::Clear, ph::ClearValue{ .depth_stencil = {1.0f, 0u} });

	auto& texture_views = scene.textures.views;

	// Only add the draw commands if a camera is set.
	if (viewport.camera() != ecs::no_entity) {
		builder.execute([this, &ifc, viewport, &scene, &texture_views](ph::CommandBuffer& cmd) {
			cmd.bind_pipeline("simple_pipeline");
			cmd.auto_viewport_scissor();

			SceneDescription::CameraInfo const& camera = scene.cameras[viewport.index()];

			ph::TypedBufferSlice<glm::mat4> cam = ifc.allocate_scratch_ubo(sizeof(glm::mat4));
			*cam.data = camera.proj_view;

			VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_info{};
			variable_count_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
			uint32_t counts[1]{ 4096 };
			variable_count_info.descriptorSetCount = 1;
			variable_count_info.pDescriptorCounts = counts;

			VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
				.add_uniform_buffer("camera", cam)
				.add_sampled_image_array("textures", texture_views, ctx.basic_sampler())
				.add_pNext(&variable_count_info)
				.get();
			cmd.bind_descriptor_set(set);

			for (auto const& draw : scene.draws) {

				// Make sure to check for null meshes
				if (!draw.mesh) {
					LOG_WRITE(LogLevel::Warning, "Draw with null mesh handle reached rendering system");
					continue;
				}

				// Don't draw the mesh if it's not ready yet.
				if (!assets::is_ready(draw.mesh)) continue;
				// Don't draw if the material isn't ready yet.
				if (!assets::is_ready(draw.material)) continue;

				gfx::Mesh const& mesh = *assets::get(draw.mesh);

				cmd.bind_vertex_buffer(0, mesh.vertices);
				cmd.bind_index_buffer(mesh.indices, VK_INDEX_TYPE_UINT32);

				cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(glm::mat4), &draw.transform);
				auto textures = scene.get_material_textures(draw.material);
				cmd.push_constants(ph::ShaderStage::Fragment, sizeof(glm::mat4), sizeof(uint32_t), &textures.albedo);
				cmd.draw_indexed(mesh.num_indices, 1, 0, 0, 0);
			}
		});
	}

	graph.add_pass(std::move(builder.get()));
}

std::vector<std::string> SimpleRenderer::debug_views(gfx::Viewport viewport) {
	return { depth_attachments[viewport.index()] };
}

void SimpleRenderer::resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) {
	if (width != 0 && height != 0)
		ctx.resize_attachment(depth_attachments[viewport.index()], { width, height });
}

} // namespace andromeda::gfx::backend