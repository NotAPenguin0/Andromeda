#pragma once

// This file exposes an interface to integrate Cascaded Shadow Mapping into a rendering pipeline.

#include <andromeda/graphics/backend/renderer_backend.hpp>

namespace andromeda::gfx::backend {

/**
 * @class CascadedShadowMapping
 * @brief This class manages resources used for CSM, and implements the creation of a shadow
 *        map creation pass.
 */
class CascadedShadowMapping {
public:
    /**
     * @brief Creates pipeline for shadowmap rendering.
     * @param ctx
     */
    static void create_pipeline(gfx::Context& ctx);

    /**
     * @brief Build the shadowmap passes for a scene. This will return as many passes as there are shadowcasting Directional Lights in the scene, multiplied by the number of
     *        cascades in each shadow map.
     * @param ctx Reference to the graphics context.
     * @param ifc Reference to the InFlightContext.
     * @param viewport Viewport we are rendering to.
     * @param scene Scene that is being rendered.
     * @return List of passes to be executed in any order.
     */
    std::vector<ph::Pass> build_shadow_map_passes(gfx::Context& ctx, ph::InFlightContext& ifc, gfx::Viewport const& viewport, gfx::SceneDescription const& scene);

    /**
     * @brief When building the shading pass you will depend on the shadow mapping to be completed.
     *        This function will properly set dependencies for all used shadow maps in the given PassBuilder.
     *        Assumes you only sample the shadow map in the fragment shader.
     * @param viewport Current rendering viewport
     * @param builder Pass builder for the depending pass that is being built.
     */
    void register_attachment_dependencies(gfx::Viewport const& viewport, ph::PassBuilder& builder);

private:
    struct Cascade {
        // ImageView to specific array layer of cascade attachment.
        ph::ImageView view = {};
        // Depth value where this cascade splits with the next cascade.
        float depth_split = 0.0f;
        // Projection matrix from light space to screen space.
        glm::mat4 light_proj_view = glm::mat4(1.0f);
    };

    struct CascadeMap {
        // Cascade 0 is the closest cascade, etc
        std::array<Cascade, ANDROMEDA_SHADOW_CASCADE_COUNT> cascades;
        // Name of the cascade attachment in the internal Phobos structure.
        std::string attachment;
    };

    struct ViewportShadowMap {
        std::array<CascadeMap, ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS> maps;
    };

    // Since this would normally result in 8 * 8 * 4 2k shadow maps, we will instead create these shadow maps on demand.
    std::array<ViewportShadowMap, gfx::MAX_VIEWPORTS> viewport_data;

    /**
     * @brief Get a cascade shadow map for the specified viewport and light index. If there was none created yet, this will create a cascade map.
     * @param ctx Reference to the graphics context.
     * @param viewport Viewport we are rendering to.
     * @param light_index Index of the light we want a cascade map for. May not be larger than or equal to ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS
     * @return
     */
    CascadeMap& request_shadow_map(gfx::Context& ctx, gfx::Viewport const& viewport, uint32_t light_index);
};

}