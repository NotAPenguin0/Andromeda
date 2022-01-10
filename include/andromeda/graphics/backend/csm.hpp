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
     * @brief Returns true if a gpu light is a shadow caster
     * @param light Light to check
     * @return True if light.direction_shadow.w >= 0, false otherwise
     */
    static bool is_shadow_caster(gpu::DirectionalLight const& light);

    /**
     * @brief Get a light index from a directional light. This maps directly to the index of a shadow map. Only meaningful if is_shadow_caster(light) returned true.
     * @param light Light to check.
     * @return Index of the light.
     */
    static uint32_t get_light_index(gpu::DirectionalLight const& light);

    /**
     * @brief Create a sampler suitable for sampling from the cascade maps.
     * @param ctx Reference to the graphics context.
     * @return VkSampler to be used when sampling from the cascade shadow maps.
     */
    static VkSampler create_sampler(gfx::Context& ctx);

    /**
     * @brief Destroy resources used by the CSM implementation
     */
    void free(gfx::Context& ctx);

    /**
     * @brief Creates pipeline for shadowmap rendering. The pipeline name will be 'shadow'
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
     * @param transforms Buffer with all scene transformation data.
     * @return List of passes to be executed in any order.
     */
    std::vector<ph::Pass> build_shadow_map_passes(gfx::Context& ctx, ph::InFlightContext& ifc, gfx::Viewport const& viewport, gfx::SceneDescription const& scene, ph::BufferSlice transforms);

    /**
     * @brief When building the shading pass you will depend on the shadow mapping to be completed.
     *        This function will properly set dependencies for all used shadow maps in the given PassBuilder.
     *        Assumes you only sample the shadow map in the fragment shader.
     * @param viewport Current rendering viewport
     * @param builder Pass builder for the depending pass that is being built.
     */
    void register_attachment_dependencies(gfx::Viewport const& viewport, ph::PassBuilder& builder);

    /**
     * @brief Obtain all debug attachments you can display
     * @param viewport Current rendering viewport.
     * @return List of names of debug attachments.
     */
    std::vector<std::string_view> get_debug_attachments(gfx::Viewport const& viewport);

    /**
     * @brief Get an ImageView pointing to the full shadow map of a given light.
     * @param viewport Current rendering viewport.
     * @param light GPU light to obtain the shadow map for.
     * @return Valid ImageView pointing to all cascades of the light's shadow map.
     */
    ph::ImageView get_shadow_map(gfx::Viewport const& viewport, gpu::DirectionalLight const& light);

    /**
     * @brief Get the split depth of a cascade.
     * @param viewport Current rendering viewport.
     * @param light GPU light to get the cascade map for.
     * @param cascade Cascade index to get the split depth for.
     * @return Depth value indicating the split depth of the given cascade of a light.
     */
    float get_split_depth(gfx::Viewport const& viewport, gpu::DirectionalLight const& light, uint32_t const cascade);

    /**
     * @brief Get the projection-view matrix of a cascade.
     * @param viewport Current rendering viewport.
     * @param light GPU light to get the cascade map for.
     * @param cascade Cascade index to get the matrix for.
     * @return 4x4 transformation matrix equal to projection * view for this cascade.
     */
    glm::mat4 get_cascade_matrix(gfx::Viewport const& viewport, gpu::DirectionalLight const& light, uint32_t const cascade);

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
        // ImageView with all cascade layers.
        ph::ImageView full_view = {};
    };

    struct ViewportShadowMap {
        std::array<CascadeMap, ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS> maps;
    };

    // Since this would normally result in 8 * 8 * 4 2k shadow maps, we will instead create these shadow maps on demand.
    std::array<ViewportShadowMap, gfx::MAX_VIEWPORTS> viewport_data;

    // Configurable value that we will leave at the default for now.
    static constexpr inline float cascade_split_lambda = 0.95f;

    /**
     * @brief Get a cascade shadow map for the specified viewport and light index. If there was none created yet, this will create a cascade map.
     * @param ctx Reference to the graphics context.
     * @param viewport Viewport we are rendering to.
     * @param light_index Index of the light we want a cascade map for. May not be larger than or equal to ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS
     * @return
     */
    CascadeMap& request_shadow_map(gfx::Context& ctx, gfx::Viewport const& viewport, uint32_t light_index);

    /**
     * @brief Updates cascade projection/view matrix and depth split distances.
     * @param shadow_map Reference to the CSM to update.
     * @param viewport Current rendering viewport.
     * @param camera Camera in the viewport we are rendering to.
     * @param light Reference to the light this map belongs to.
     */
    void calculate_cascade_splits(CascadeMap& shadow_map, gfx::Viewport const& viewport, gfx::SceneDescription::CameraInfo const& camera, gpu::DirectionalLight const& light);
};

}