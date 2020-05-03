#include <andromeda/renderer/renderer.hpp>

#include <phobos/renderer/renderer.hpp>
#include <phobos/present/present_manager.hpp>
#include <phobos/core/vulkan_context.hpp>
#include <phobos/pipeline/shader_info.hpp>
#include <stl/literals.hpp>

#include <andromeda/util/version.hpp>
#include <andromeda/util/log.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/assets/texture.hpp>
#include <andromeda/world/world.hpp>

#include <andromeda/components/dbg_quad.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/components/transform.hpp>
#include <andromeda/components/mesh_renderer.hpp>

#include <andromeda/util/math.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	pci.blend_attachments.push_back(blend);
	pci.blend_logic_op_enable = false;
#if ANDROMEDA_DEBUG
	pci.debug_name = "generic_pipeline";
#endif
	pci.dynamic_states.push_back(vk::DynamicState::eViewport);
	pci.dynamic_states.push_back(vk::DynamicState::eScissor);
	
	pci.viewports.emplace_back();
	pci.scissors.emplace_back();

	ph::ShaderHandle vert = ph::create_shader(*ctx.vulkan, ph::load_shader_code("data/shaders/generic.vert.spv"), "main", vk::ShaderStageFlagBits::eVertex);
	ph::ShaderHandle frag = ph::create_shader(*ctx.vulkan, ph::load_shader_code("data/shaders/generic.frag.spv"), "main", vk::ShaderStageFlagBits::eFragment);

	pci.shaders.push_back(vert);
	pci.shaders.push_back(frag);

	pci.vertex_input_binding.binding = 0;
	pci.vertex_input_binding.stride = 5 * sizeof(float);
	pci.vertex_input_binding.inputRate = vk::VertexInputRate::eVertex;

	pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32B32Sfloat, 0_u32);
	pci.vertex_attributes.emplace_back(1_u32, 0_u32, vk::Format::eR32G32Sfloat, (uint32_t)(3 * sizeof(float)));

	pci.rasterizer.cullMode = vk::CullModeFlagBits::eNone;

	ph::reflect_shaders(*ctx.vulkan, pci);

	cam_binding = pci.shader_info["camera"];
	transform_binding = pci.shader_info["transforms"];
	tex_binding = pci.shader_info["textures"];

	ctx.vulkan->pipelines.create_named_pipeline("generic", std::move(pci));
}

Renderer::~Renderer() {
	vk_renderer->destroy();
	vk_present->destroy();
}

static constexpr float quad_verts[] = {
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f
};

void Renderer::render(Context& ctx) {
	using namespace components;

	vk_present->wait_for_available_frame();

	ph::FrameInfo& frame = vk_present->get_frame_info();
	ph::RenderGraph graph{ ctx.vulkan.get() };

	// Reset render database, we will fill this again each frame
	database.reset();

	// Fill render database

	// For now, we populate the render database with all loaded materials. Later, we can improve this to only include used materials
	for (auto const& [id, _] : assets::storage::data<Material>) {
		database.add_material(Handle<Material>{ id });
	}

	for (auto const& [transform, rend, _] : ctx.world->ecs().view<Transform, MeshRenderer, DbgQuad>()) {
		// Calculate model matrix from transformation
		glm::mat4 model = glm::translate(glm::mat4(1.0), transform.position);
		model = glm::rotate(model, { glm::radians(transform.rotation.x),
								glm::radians(transform.rotation.y),
								glm::radians(transform.rotation.z) });
		model = glm::scale(model, transform.scale);
		// Temporary default material
		database.add_draw(renderer::Draw{ .material = rend.material, .transform = model });
	}

	// This renderpass is temporary, we will improve on this system when adding ImGui

	ph::RenderPass present_pass;
#if ANDROMEDA_DEBUG
	present_pass.debug_name = "present_pass";
#endif
	present_pass.clear_values = { vk::ClearColorValue{ std::array<float, 4>{ {0.0f, 0.0f, 0.0f, 1.0f}} } };
	
	ph::RenderAttachment swapchain = vk_present->get_swapchain_attachment(frame);
	present_pass.outputs = { swapchain };
	present_pass.callback = [this, &ctx, &frame, &swapchain](ph::CommandBuffer& cmd_buf) {
		ph::BufferSlice cam_buffer = cmd_buf.allocate_scratch_ubo(sizeof(glm::mat4));
		for (auto const&[trans, cam] : ctx.world->ecs().view<Transform, Camera>()) {
			glm::mat4 projection = glm::perspective(cam.fov, (float)swapchain.get_width() / (float)swapchain.get_height(), 0.1f, 100.0f);
			projection[1][1] *= -1; // Vulkan has upside down projection
			glm::mat4 view = glm::lookAt(trans.position, trans.position + cam.front, cam.up);
			glm::mat4 projection_view = projection * view;
			std::memcpy(cam_buffer.data, glm::value_ptr(projection_view), sizeof(glm::mat4));
			break;
		}

		size_t transforms_size = database.transforms.size();
		ph::BufferSlice transforms = cmd_buf.allocate_scratch_ssbo(transforms_size * sizeof(glm::mat4));
		std::memcpy(transforms.data, database.transforms.data(), transforms_size * sizeof(glm::mat4));

		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(tex_binding, database.texture_views, frame.default_sampler));
		set_binding.add(ph::make_descriptor(transform_binding, transforms));
		set_binding.add(ph::make_descriptor(cam_binding, cam_buffer));
		
		vk::DescriptorSetVariableDescriptorCountAllocateInfo variable_count_info;
		uint32_t counts[1]{ ph::meta::max_unbounded_array_size };
		variable_count_info.descriptorSetCount = 1;
		variable_count_info.pDescriptorCounts = counts;

		vk::Viewport vp;
		vp.x = 0;
		vp.y = 0;
		vp.width = swapchain.get_width();
		vp.height = swapchain.get_height();
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		cmd_buf.set_viewport(vp);
		vk::Rect2D scissor;
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = vk::Extent2D{ swapchain.get_width(), swapchain.get_height() };
		cmd_buf.set_scissor(scissor);

		ph::RenderPass& pass = *cmd_buf.get_active_renderpass();
		ph::Pipeline pipeline = vk_renderer->get_pipeline("generic", pass);
		cmd_buf.bind_pipeline(pipeline);

		vk::DescriptorSet set = vk_renderer->get_descriptor(frame, pass, set_binding, &variable_count_info);
		cmd_buf.bind_descriptor_set(0, set);

		ph::BufferSlice mesh = cmd_buf.allocate_scratch_vbo(sizeof(quad_verts));
		std::memcpy(mesh.data, quad_verts, sizeof(quad_verts));
		cmd_buf.bind_vertex_buffer(0, mesh);

		for (uint32_t i = 0; i < database.draws.size(); ++i) {
			auto const& draw = database.draws[i];
			auto texture_indices = database.get_material_textures(draw.material);
			uint32_t const transform_idx = i;
			uint32_t const tex_idx = texture_indices.diffuse;
			cmd_buf.push_constants(vk::ShaderStageFlagBits::eVertex, 0, sizeof(uint32_t), &transform_idx);
			cmd_buf.push_constants(vk::ShaderStageFlagBits::eFragment, sizeof(uint32_t), sizeof(uint32_t), &tex_idx);
			// Draw quads for now
			cmd_buf.draw(6, 1, 0, 0);
		}
	};

	graph.add_pass(std::move(present_pass));

	graph.build();

	frame.render_graph = &graph;
	vk_renderer->render_frame(frame);
	vk_present->present_frame(frame);
}

}