#ifndef ANDROMEDA_RENDERER_HPP_
#define ANDROMEDA_RENDERER_HPP_

#include <andromeda/wsi/window.hpp>
#include <andromeda/core/context.hpp>
#include <andromeda/renderer/render_database.hpp>
#include <andromeda/renderer/deferred/geometry_pass.hpp>
#include <andromeda/renderer/deferred/lighting_pass.hpp>
#include <andromeda/renderer/deferred/skybox_pass.hpp>
#include <andromeda/renderer/deferred/tonemap_pass.hpp>

#include <phobos/renderer/render_attachment.hpp>
#include <phobos/pipeline/pipeline.hpp>

namespace andromeda::renderer {

class Renderer {
public:
	Renderer(Context& ctx);
	virtual ~Renderer();

	// Renders the world (and UI) to the screen. Before calling this, you are not allowed to directly modify in-use GPU resources.
	void render(Context& ctx);
	ph::ImageView scene_image() const { return color_final->image_view(); }

	virtual ph::ImageView debug_image() const = 0;

protected:
	virtual void render_frame(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph) = 0;

	std::unique_ptr<ph::Renderer> vk_renderer;
	std::unique_ptr<ph::PresentManager> vk_present;

	RenderDatabase database;
	ph::RenderAttachment* color_final = nullptr;
};

class DeferredRenderer : public Renderer {
public:
	DeferredRenderer(Context& ctx);
	virtual ~DeferredRenderer();

	deferred::GeometryPass& get_geometry_pass() { return *geometry_pass; }
	deferred::LightingPass& get_lighting_pass() { return *lighting_pass; }

private:
	void render_frame(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph) override;

	std::unique_ptr<deferred::GeometryPass> geometry_pass;
	std::unique_ptr<deferred::LightingPass> lighting_pass;
	std::unique_ptr<deferred::SkyboxPass> skybox_pass;
	std::unique_ptr<deferred::TonemapPass> tonemap_pass;

	ph::RenderAttachment* scene_color;
};

class ForwardPlusRenderer : public Renderer {
public:
	ForwardPlusRenderer(Context& ctx);
	virtual ~ForwardPlusRenderer() = default;

	ph::ImageView debug_image() const override { return depth->image_view(); }

private:
	void create_pipelines(Context& ctx);

	void render_frame(Context& ctx, ph::FrameInfo& frame, ph::RenderGraph& graph) override;

	void update_transforms(ph::CommandBuffer& cmd_buf);
	void update_camera_data(ph::CommandBuffer& cmd_buf);
	void update_lights(ph::CommandBuffer& cmd_buf);
	void create_compute_output_buffer(ph::CommandBuffer& cmd_buf);

	void depth_prepass(ph::RenderGraph& graph, Context& ctx);
	void light_cull(ph::RenderGraph& graph, Context& ctx);

	ph::RenderAttachment* depth;
	vk::Sampler depth_sampler;

	struct DepthBindings {
		ph::ShaderInfo::BindingInfo camera;
		ph::ShaderInfo::BindingInfo transforms;
	} depth_bindings{};

	struct ComputeBindings {
		ph::ShaderInfo::BindingInfo depth;
		ph::ShaderInfo::BindingInfo camera;
		ph::ShaderInfo::BindingInfo lights;
		ph::ShaderInfo::BindingInfo visible_lights_output;
	} compute_bindings{};

	struct PerFrameBuffers {
		ph::BufferSlice camera;
		ph::BufferSlice transforms;
		ph::BufferSlice lights;
		ph::BufferSlice visible_light_indices;
	} per_frame_buffers{};
};

} // namespace andromeda::renderer

#endif