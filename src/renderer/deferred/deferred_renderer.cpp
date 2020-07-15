#include <andromeda/renderer/renderer.hpp>

#include <phobos/present/present_manager.hpp>

namespace andromeda::renderer {

DeferredRenderer::DeferredRenderer(Context& ctx) : Renderer(ctx) {
	geometry_pass = std::make_unique<deferred::GeometryPass>(ctx, *vk_present);
	lighting_pass = std::make_unique<deferred::LightingPass>(ctx, *vk_present);
	skybox_pass = std::make_unique<deferred::SkyboxPass>(ctx, *vk_present);
	tonemap_pass = std::make_unique<deferred::TonemapPass>(ctx);

	scene_color = &vk_present->add_color_attachment("scene_color", { 1280, 720 }, vk::Format::eR16G16B16A16Sfloat);
}

DeferredRenderer::~DeferredRenderer() {

}

void DeferredRenderer::render_frame(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph) {
	geometry_pass->build(ctx, frame, graph, database);
	lighting_pass->build(ctx, {
			.output = *scene_color,
			.depth = geometry_pass->get_depth(),
			.albedo_ao = geometry_pass->get_albedo_ao(),
			.metallic_roughness = geometry_pass->get_metallic_roughness(),
			.normal = geometry_pass->get_normal()
		}, frame, graph, database);
	skybox_pass->build(ctx, {
			.output = *scene_color,
			.depth = lighting_pass->get_resolved_depth()
		}, frame, graph, database);
	tonemap_pass->build(ctx, {
			.input_hdr = *scene_color,
			.output_ldr = *color_final
		}, frame, graph, database);
}

}