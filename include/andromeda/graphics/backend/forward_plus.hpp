#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>
#include <array>

namespace andromeda::gfx::backend {

/**
 * @class ForwardPlusRenderer
 * @brief Rendering backend implementing Forward+ rendering.
 */
class ForwardPlusRenderer : public RendererBackend {
public:
    explicit ForwardPlusRenderer(gfx::Context& ctx);

    /**
     * @brief Renders a scene to a viewport.
     * @param graph Render graph to add the renderpasses to.
     * @param ifc In-flight context for allocating scratch buffers.
     * @param viewport Viewport to render to.
     * @param scene SceneDescription with all necessary information.
    */
    void render_scene(ph::RenderGraph& graph, ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) override;

    /**
	 * @brief Get a list of debug views associated with a viewport.
	 * @param viewport The viewport to get debug views for.
	 * @return A vector of strings, each element being the name of a debug attachment that can be displayed.
	 *		   SimpleRenderer only exposes a depth attachment.
	*/
    std::vector<std::string> debug_views(gfx::Viewport viewport) override;

    /**
     * @brief Called by the renderer when a viewport is resized.
     * @param viewport The viewport that is being resized.
     * @param width The new width of the viewport.
     * @param height The new height of the viewport
    */
    void resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) override;
private:
    // Depth attachments for every viewport
    std::array<std::string, gfx::MAX_VIEWPORTS> depth_attachments;

    // This structure owns buffers and storage images shared by the pipeline.
    struct RenderData {
        ph::TypedBufferSlice<glm::mat4> camera_ubo;
        ph::TypedBufferSlice<gpu::PointLight> point_lights; // SSBO
        ph::BufferSlice culled_lights; // SSBO
    } render_data;

    void create_render_data(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene);

    ph::Pass depth_prepass(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene);
    ph::Pass light_cull(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene);
};

}