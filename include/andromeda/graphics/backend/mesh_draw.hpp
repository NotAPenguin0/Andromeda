#pragma once

#include <functional>
#include <andromeda/graphics/backend/renderer_backend.hpp>

// We will have a couple loops like (simplified)
/*
 *  for (uint32_t i = 0; i < scene.draws.size(); ++i) {
 *      auto const& draw = scene.draws[i];
 *      if (!draw.mesh()) log_null_mesh();
 *      if (!assets::is_ready(draw.mesh)) continue;
 *      gfx::Mesh const& mesh = *assets::get(draw.mesh);
 *      // do something with mesh and draw index
 *  }
 */
// To reduce the effort of writing this loop every time, we simply abstract it here

namespace andromeda::gfx::backend {

/**
 * @brief Executes a certain function for each mesh that is ready to draw.
 *        This does not check readiness of materials.
 * @param scene Scene to render
 * @param callback Function to call for each ready mesh. The first parameter is the draw being executed, the second is a reference to a ready mesh, the third is the draw index.
 */
void for_each_ready_mesh(gfx::SceneDescription const& scene, std::function<void(SceneDescription::Draw const&, gfx::Mesh const&, uint32_t)> const& callback);

/**
 * @brief Binds mesh vertex and index buffers and records a drawcall.
 * @param cmd Command buffer to record to.
 * @param mesh Mesh to draw
 */
void bind_and_draw(ph::CommandBuffer& cmd, gfx::Mesh const& mesh);

}