#include <andromeda/graphics/backend/rtx.hpp>

namespace andromeda::gfx::backend {

SceneAccelerationStructure::SceneAccelerationStructure(gfx::Context& ctx) : ctx(ctx) {

}

SceneAccelerationStructure::~SceneAccelerationStructure() {

}

void SceneAccelerationStructure::update(gfx::SceneDescription const& scene) {
    // 1) We destroy items in the BLAS destroy queue.

}

[[nodiscard]] VkAccelerationStructureKHR SceneAccelerationStructure::get_acceleration_structure() const {
    return top_level[tlas_index].as.handle;
}

[[nodiscard]] VkSemaphore SceneAccelerationStructure::get_tlas_build_semaphore() const {
    return top_level[tlas_index].build_completed;
}

std::vector<Handle<gfx::Mesh>> SceneAccelerationStructure::find_unique_meshes(gfx::SceneDescription const& scene) {
    std::vector<Handle<gfx::Mesh>> result;
    // TODO: Slow algorithm, maybe optimize?
    for (auto const& draw : scene.get_draws()) {
        if (assets::is_ready(draw.mesh) && std::find(result.begin(), result.end(), draw.mesh) == result.end()) {
            result.push_back(draw.mesh);
        }
    }
    return result;
}

}