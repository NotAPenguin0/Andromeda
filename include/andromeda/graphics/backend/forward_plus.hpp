#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>
#include <andromeda/graphics/backend/atmosphere.hpp>
#include <andromeda/graphics/backend/rtx.hpp>
#include <array>

namespace andromeda::gfx::backend {

/**
 * @class ForwardPlusRenderer
 * @brief Rendering backend implementing Forward+ rendering.
 */
class ForwardPlusRenderer : public RendererBackend {
public:
    explicit ForwardPlusRenderer(gfx::Context& ctx);

    ~ForwardPlusRenderer() override;

    void frame_setup(ph::InFlightContext& ifc, gfx::SceneDescription const& scene) override;

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

    std::vector<ph::WaitSemaphore> wait_semaphores() override;

    /**
     * @brief Called by the renderer when a viewport is resized.
     * @param viewport The viewport that is being resized.
     * @param width The new width of the viewport.
     * @param height The new height of the viewport
    */
    void resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) override;

private:
    inline static constexpr uint32_t MAX_TEXTURES = 4096;

    // HDR color attachments for every viewport
    std::array<std::string, gfx::MAX_VIEWPORTS> color_attachments;
    // Depth attachments for every viewport
    std::array<std::string, gfx::MAX_VIEWPORTS> depth_attachments;
    // Heatmap attachments
    std::array<std::string, gfx::MAX_VIEWPORTS> heatmaps;
    // shadow history buffer for simple temporal denoising
    std::array<std::string, 2 * gfx::MAX_VIEWPORTS> shadow_history;

    // This structure owns buffers and storage images shared by the pipeline.
    struct RenderData {
        explicit inline RenderData(gfx::Context& ctx) : accel_structure(ctx), atmosphere(ctx, msaa_samples, msaa_sample_ratio) {}

        // Per-viewport render data, indexed by viewport ID.
        struct PerViewport {
            ph::BufferSlice camera;
            ph::BufferSlice culled_lights;
            uint32_t n_tiles_x = 0;
            uint32_t n_tiles_y = 0;

            ph::RawBuffer average_luminance;

            // frame number used to denoise shadows.
            uint32_t frame = 0;
        } vp[gfx::MAX_VIEWPORTS];

        // Can be shared by all viewports.
        ph::TypedBufferSlice<gpu::PointLight> point_lights;
        ph::TypedBufferSlice<gpu::DirectionalLight> dir_lights;
        ph::TypedBufferSlice<gpu::CascadeMapInfo> cascade_infos;
        ph::TypedBufferSlice<glm::mat4> transforms;

        // Note that casting this to uint32_t gives back the amount of samples
        VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_8_BIT;
        float msaa_sample_ratio = 0.1f;

        SceneAccelerationStructure accel_structure;
        AtmosphereRendering atmosphere;
    } render_data;

    void create_render_data(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene);

    ph::Pass light_cull(ph::InFlightContext& ifc, gfx::Viewport const& viewport, gfx::SceneDescription const& scene);

    ph::Pass shading(ph::InFlightContext& ifc, gfx::Viewport const& viewport, gfx::SceneDescription const& scene);
};

}