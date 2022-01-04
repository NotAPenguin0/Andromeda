#include <andromeda/graphics/backend/mesh_draw.hpp>

namespace andromeda::gfx::backend {

void for_each_ready_mesh(gfx::SceneDescription const& scene, std::function<void(SceneDescription::Draw const&, gfx::Mesh const&, uint32_t)> const& callback) {
    auto const& draws = scene.get_draws();
    for (uint32_t i = 0; i < draws.size(); ++i) {
        auto const& draw = draws[i];
        // Make sure to check for null meshes
        if (!draw.mesh) {
            LOG_WRITE(LogLevel::Warning, "Draw with null mesh handle reached rendering system");
            continue;
        }

        if (!assets::is_ready(draw.mesh)) continue;
        gfx::Mesh const& mesh = *assets::get(draw.mesh);
        callback(draw, mesh, i);
    }
}

void bind_and_draw(ph::CommandBuffer& cmd, gfx::Mesh const& mesh) {
    cmd.bind_vertex_buffer(0, mesh.vertices);
    cmd.bind_index_buffer(mesh.indices, VK_INDEX_TYPE_UINT32);
    cmd.draw_indexed(mesh.num_indices, 1, 0, 0, 0);
}

}