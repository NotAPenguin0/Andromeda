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

#include <andromeda/components/static_mesh.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/components/point_light.hpp>
#include <andromeda/components/transform.hpp>
#include <andromeda/components/mesh_renderer.hpp>

#include <andromeda/util/math.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui_impl_phobos.h>

namespace andromeda::renderer {

static ph::Texture create_single_color_texture(ph::VulkanContext& ctx, uint8_t r, uint8_t g, uint8_t b, vk::Format fmt) {
	ph::Texture::CreateInfo info;
	info.channels = 4;
	info.ctx = &ctx;
	const uint8_t data[] = { r, g, b, 255 };
	info.data = data;
	info.width = 1;
	info.height = 1;
	info.format = fmt;
	return ph::Texture(info);
}

static ph::Cubemap create_single_color_cube(ph::VulkanContext& ctx, uint8_t r, uint8_t g, uint8_t b, vk::Format fmt) {
	ph::Cubemap::CreateInfo info;
	info.channels = 4;
	info.ctx = &ctx;
	info.width = 1;
	info.height = 1;
	const uint8_t data[] = { r, g, b, 255 };
	for (auto &face : info.faces) {
		face = data;
	}
	info.format = fmt;

	return ph::Cubemap(info);
}

Renderer::Renderer(Context& ctx) {
	vk_present = std::make_unique<ph::PresentManager>(*ctx.vulkan);
	vk_renderer = std::make_unique<ph::Renderer>(*ctx.vulkan);

	render_size = { 1920, 1080 };

	color_final = &vk_present->add_color_attachment("color_final", render_size);
	attachments.push_back(color_final);

	default_textures.push_back(create_single_color_texture(*ctx.vulkan, 255, 0, 255, vk::Format::eR8G8B8A8Srgb)); // color
	default_textures.push_back(create_single_color_texture(*ctx.vulkan, 0, 255, 0, vk::Format::eR8G8B8A8Unorm)); // normal
	default_textures.push_back(create_single_color_texture(*ctx.vulkan, 0, 0, 0, vk::Format::eR8G8B8A8Unorm)); // metallic
	default_textures.push_back(create_single_color_texture(*ctx.vulkan, 0, 0, 0, vk::Format::eR8G8B8A8Unorm)); // roughness
	default_textures.push_back(create_single_color_texture(*ctx.vulkan, 255, 255, 255, vk::Format::eR8G8B8A8Unorm)); // ambient occlusion

	database.default_color = default_textures[0].view_handle();
	database.default_normal = default_textures[1].view_handle();
	database.default_metallic = default_textures[2].view_handle();
	database.default_roughness = default_textures[3].view_handle();
	database.default_ambient_occlusion = default_textures[4].view_handle();

	default_cube = create_single_color_cube(*ctx.vulkan, 0, 0, 0, vk::Format::eR8G8B8A8Srgb);
	database.default_skybox = default_cube.view_handle();
	database.default_irr_map = default_cube.view_handle();
	database.default_specular_map = default_cube.view_handle();
}

Renderer::~Renderer() {
	vk_renderer->destroy();
	vk_present->destroy();

	for (auto& tex : default_textures) {
		tex.destroy();
	}
	default_textures.clear();
	default_cube.destroy();
}

void Renderer::render(Context& ctx) {
	using namespace components;

	vk_present->wait_for_available_frame();

	ph::FrameInfo& frame = vk_present->get_frame_info();
	ph::RenderGraph graph{ ctx.vulkan.get(), &ctx.vulkan->thread_contexts[0] };

	// Reset render database, we will fill this again each frame
	database.reset();

	// Fill render database
	for (auto const& [trans, cam] : ctx.world->ecs().view<Transform, Camera>()) {
		database.projection = glm::perspective(cam.fov, (float)color_final->get_width() / (float)color_final->get_height(), 0.1f, 100.0f);
		database.projection[1][1] *= -1; // Vulkan has upside down projection
		database.view = glm::lookAt(trans.position, trans.position + cam.front, cam.up);
		database.projection_view = database.projection * database.view;
		database.camera_position = trans.position;
		database.environment_map = cam.env_map;
		break;
	}

	// For now, we populate the render database with all loaded materials. Later, we can improve this to only include used materials
	for (auto const& [id, _] : assets::storage::data<Material>) {
		database.add_material(Handle<Material>{ id });
	}

	for (auto const& [transform, rend, mesh] : ctx.world->ecs().view<Transform, MeshRenderer, StaticMesh>()) {
		// Calculate model matrix from transformation
		glm::mat4 model = glm::translate(glm::mat4(1.0), transform.position);
		model = glm::rotate(model, { 
			glm::radians(transform.rotation.x),
			glm::radians(transform.rotation.y),
			glm::radians(transform.rotation.z) 
		});
		model = glm::scale(model, transform.scale);
		database.add_draw(renderer::Draw{ .mesh = mesh.mesh, .material = rend.material, .transform = model });
	}

	for (auto const& [trans, light] : ctx.world->ecs().view<Transform, PointLight>()) {
		database.add_point_light(trans.position, light.radius, light.color, light.intensity);
	}

	render_frame(ctx, frame, graph);

	ImGui::Render();
	ImGui_ImplPhobos_RenderDrawData(ImGui::GetDrawData(), &frame, &graph, vk_renderer.get());

	graph.build();

	frame.render_graph = &graph;
	vk_renderer->render_frame(frame);
	vk_present->present_frame(frame);
}

void Renderer::resize(vk::Extent2D size) {
	render_size = size;
	for (auto attachment : attachments) {
		attachment->resize(size.width, size.height);
	}
}

}