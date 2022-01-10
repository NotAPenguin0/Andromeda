#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>

// This file provides utility functions to create a depth-only pass.

namespace andromeda::gfx::backend {

/**
 * @brief Creates a pipeline with name 'depth_only' that can be used to only render depth.
 * @param ctx Reference to the graphics context.
 * @param samples Number of MSAA samples.
 * @param sample_ratio Ratio used for MSAA shading.
 */
void create_depth_only_pipeline(gfx::Context& ctx, VkSampleCountFlagBits samples, float sample_ratio);

/**
 * @brief Build a depth-only renderpass object and return it.
 * @param ctx Reference to the graphics context.
 * @param ifc Reference to an InFlightContext used to allocate scratch memory.
 * @param target Name of the depth attachment to render to.
 * @param scene Scene to render.
 * @param camera BufferSlice with camera data.
 * @param transforms BufferSlice with transform data.
 * @return ph::Pass object describing the depth pass.
 */
ph::Pass build_depth_pass(gfx::Context& ctx, ph::InFlightContext& ifc, std::string_view target, gfx::SceneDescription const& scene, ph::BufferSlice camera, ph::BufferSlice transforms);

}