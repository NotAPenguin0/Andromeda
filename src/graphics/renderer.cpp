#include <andromeda/graphics/renderer.hpp>

#include <andromeda/graphics/backend/simple.hpp>
#include <andromeda/graphics/imgui.hpp>

#include <andromeda/components/transform.hpp>
#include <andromeda/components/mesh_renderer.hpp>
#include <andromeda/components/hierarchy.hpp>

#include <phobos/render_graph.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace glm {
// Extension for glm to rotate correctly around multiple axis using euler angles
mat4 rotate(mat4 const& mat, vec3 euler) {
	mat4 result;
	result = rotate(mat, euler.z, vec3{ 0, 0, 1 });
	result = rotate(result, euler.y, vec3{ 0, 1, 0 });
	result = rotate(result, euler.x, vec3{ 1, 0, 0 });
	return result;
}
}

namespace andromeda{
namespace gfx {

Renderer::Renderer(gfx::Context& ctx, Window& window) {
	gfx::imgui::init(ctx, window);

	// Initialize viewports
	for (uint32_t i = 0; i < gfx::MAX_VIEWPORTS; ++i) {
		gfx::Viewport& vp = viewports[i].vp;
		vp.id = i;
		vp.target_name = gfx::Viewport::local_string(vp, "rendertarget");
		vp.width_px = window.width();
		vp.height_px = window.height();
		
		ctx.create_attachment(vp.target(), { vp.width(), vp.height() }, VK_FORMAT_R8G8B8A8_SRGB);
	}

	impl = std::make_unique<backend::SimpleRenderer>(ctx);
}

void Renderer::shutdown(gfx::Context& ctx) {
	gfx::imgui::shutdown();
}

void Renderer::render_frame(gfx::Context& ctx, World const& world) {
	ph::InFlightContext ifc = ctx.wait_for_frame();
	
	// Reset scene description from last frame
	scene.reset();

	fill_scene_description(world);

	ph::Pass clear_swap = ph::PassBuilder::create("clear")
		.add_attachment(ctx.get_swapchain_attachment_name(), 
						ph::LoadOp::Clear, { .color = {0.0f, 0.0f, 0.0f, 1.0f} })
		.get();

	ph::RenderGraph graph{};
	graph.add_pass(std::move(clear_swap));

	for (auto const& viewport : viewports) {
		if (viewport.in_use) {
			impl->render_scene(graph, ifc, viewport.vp, scene);
		}
	}

	// After all passes we add the imgui render pass
	imgui::render(graph, ifc, ctx.get_swapchain_attachment_name());

	graph.build(ctx);

	ph::RenderGraphExecutor executor{};
	ifc.command_buffer.begin();
	executor.execute(ifc.command_buffer, graph);
	ifc.command_buffer.end();

	ctx.submit_frame_commands(*ctx.get_queue(ph::QueueType::Graphics), ifc.command_buffer);
	ctx.present(*ctx.get_present_queue());
}

std::optional<gfx::Viewport> Renderer::create_viewport(uint32_t width, uint32_t height, ecs::entity_t camera) {
	// Find the first viewport slot that's not in use
	auto it = viewports.begin();
	while (it != viewports.end()) {
		// Found a free slot.
		if (!it->in_use) break;
		++it;
	}

	// No free slots found
	if (it == viewports.end()) {
		return std::nullopt;
	}

	it->vp.camera_id = camera;
	it->in_use = true;
	return it->vp;
}

void Renderer::resize_viewport(gfx::Context& ctx, gfx::Viewport& viewport, uint32_t width, uint32_t height) {
	// Don't resize when one parameter is zero.
	if (width != 0 && height != 0) {
		ctx.resize_attachment(viewport.target(), { width, height });
		// Update viewport information
		viewport.width_px = width;
		viewport.height_px = height;

		viewports[viewport.index()].vp.width_px = width;
		viewports[viewport.index()].vp.height_px = height;

		impl->resize_viewport(viewport, width, height);
	}
}

std::vector<gfx::Viewport> Renderer::get_active_viewports() {
	std::vector<gfx::Viewport> result{};
	result.reserve(gfx::MAX_VIEWPORTS);
	for (auto const& viewport : viewports) {
		if (viewport.in_use) {
			result.push_back(viewport.vp);
		}
	}
	return result;
}

void Renderer::set_viewport_camera(gfx::Viewport& viewport, ecs::entity_t camera) {
	viewport.camera_id = camera;
	viewports[viewport.index()].vp.camera_id = camera;
}

void Renderer::destroy_viewport(gfx::Viewport viewport) {
	viewports[viewport.index()].in_use = false;
}

std::vector<std::string> Renderer::get_debug_views(gfx::Viewport viewport) {
	return impl->debug_views(viewport);
}

/**
 * @brief Computes the transform matrix of entity's local space to world space by applying parent transforms.
 * @param ent Entity to compute local to world matrix for.
 * @param ecs ECS registry.
 * @param lookup Lookup table with past results to avoid repeatedly computing parent transforms.
 * @return A matrix transforming an entity's local space to world space.
*/
static glm::mat4 local_to_world(ecs::entity_t ent, thread::LockedValue<ecs::registry const> const& ecs, 
	std::unordered_map<ecs::entity_t, glm::mat4>& lookup) {

	// Look up this entity in the lookup table first, we might have already computed this transform.
	if (auto it = lookup.find(ent); it != lookup.end()) {
		return it->second;
	}

	Hierarchy const& hierarchy = ecs->get_component<Hierarchy>(ent);
	ecs::entity_t parent = hierarchy.parent;

	glm::mat4 parent_transform = glm::mat4(1.0);

	if (parent != ecs::no_entity) {
		parent_transform = local_to_world(parent, ecs, lookup);
	}

	// The following algorithm is based on section 2 from
	// https://docs.unity3d.com/Packages/com.unity.entities@0.0/manual/transform_system.html

	// Now that we have the parent to world matrix (or an identity matrix if this entity has no parent),
	// we simply apply this entity's transformation.
	Transform const& transform = ecs->get_component<Transform>(ent);
	glm::mat4 local_transform = glm::translate(glm::mat4(1.0), transform.position);
	local_transform = glm::rotate(local_transform, {
		glm::radians(transform.rotation.x),
		glm::radians(transform.rotation.y),
		glm::radians(transform.rotation.z)
	});
	local_transform = glm::scale(local_transform, transform.scale);

	glm::mat4 local_to_world = parent_transform * local_transform;
	// Store the transform in the lookup table.
	lookup[ent] = local_to_world;

	return local_to_world;
}

void Renderer::fill_scene_description(World const& world) {
	// TODO: Transform entities based on their parent transform.

	// Access the ECS.
	auto ecs = world.ecs();
	
	std::unordered_map<ecs::entity_t, glm::mat4> transform_lookup{};

	// Add all meshes in the world to the draw list
	for (auto [_, mesh, hierarchy] : ecs->view<Transform, MeshRenderer, Hierarchy>()) {
		glm::mat4 transform = local_to_world(hierarchy.this_entity, ecs, transform_lookup);
		scene.add_draw(mesh.mesh, transform);
	}

	// Add every camera/viewport combo.
	for (auto const& viewport : viewports) {
		if (viewport.in_use) {
			ecs::entity_t camera = viewport.vp.camera();
			// Don't render viewports with no camera
			if (camera == ecs::no_entity) continue;
			scene.add_viewport(viewport.vp, ecs->get_component<Transform>(camera), ecs->get_component<Camera>(camera));
		}
	}
}

} // namespace gfx
} // namespace andromeda