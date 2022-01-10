#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>

namespace andromeda::gfx::backend {

/**
 * @brief Creates pipelines for the luminance histogram calculation.
 *        This will create two compute pipelines, 'luminance_accumulate' and 'luminance_average'
 * @param ctx Reference to the graphics context.
 */
void create_luminance_histogram_pipelines(gfx::Context& ctx);

/**
 * @brief Creates a named pipeline for the tonemapping pass with name 'tonemap'.
 * @param ctx Reference to the graphics context.
 */
void create_tonemapping_pipeline(gfx::Context& ctx);

/**
 * @brief Builds a compute-only pass that will calculate the average luminance in the scene and store it in the GPU buffer 'average'.
 * @param ctx Reference to the graphics context.
 * @param ifc Reference to the InFlightContext.
 * @param hdr_scene Name of the main HDR color attachment.
 * @param viewport Viewport to render to.
 * @param scene Scene to calculate the average luminance of.
 * @param average GPU buffer where the average luminance will be stored.
 * @return ph::Pass object with the luminance pass.
 */
ph::Pass build_average_luminance_pass(gfx::Context& ctx, ph::InFlightContext& ifc, std::string_view main_hdr, gfx::Viewport const& viewport,
                                      gfx::SceneDescription const& scene, ph::BufferSlice average);


/**
 * @brief Builds a tonemapping pass that renders the scene HDR to an sRGB target attachment. Also executes the exposure correction and resolves MSAA.
 * @param ctx Reference to the graphics context.
 * @param main_hdr Name of the main HDR color attachment.
 * @param output Name of the target attachment (should be sRGB for correct results).
 * @param samples Number of samples the main color attachment has.
 * @param luminance GPU buffer with the average luminance value in the scene.
 * @return ph::Pass object with the luminance pass.
 */
ph::Pass build_tonemap_pass(gfx::Context& ctx, std::string_view main_hdr, std::string_view target, VkSampleCountFlagBits samples, ph::BufferSlice luminance);

}