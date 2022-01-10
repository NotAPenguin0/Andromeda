#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>

namespace andromeda::gfx::backend {

/**
 * @brief Creates a named pipeline in ctx with name 'skybox' used to render skyboxes with render_skybox().
 * @param ctx Reference to the graphics context.
 * @param samples Number of MSAA samples.
 * @param sample_ratio Ratio used for MSAA shading.
 */
void create_skybox_pipeline(gfx::Context& ctx, VkSampleCountFlagBits samples, float sample_ratio);

/**
 * @brief Record commands to render a skybox to a given command buffer.
 * @param ctx Reference to the graphics context.
 * @param ifc Reference to the InFlightContext.
 * @param cmd Reference to the command buffer to record to.
 * @param env Environment map to use when rendering the skybox.
 * @param camera Camera data for the current viewport.
 */
void render_skybox(gfx::Context& ctx, ph::InFlightContext& ifc, ph::CommandBuffer& cmd, gfx::Environment const& env, SceneDescription::CameraInfo const& camera);

}