#include <andromeda/graphics/backend/rtx.hpp>

namespace andromeda::gfx::backend {

SceneAccelerationStructure::SceneAccelerationStructure(gfx::Context& ctx) : ctx(ctx) {
    accel_structure = std::make_unique<AccelerationStructure>(ctx);
    async_update_accel_structure = std::make_unique<AccelerationStructure>(ctx);
}

void SceneAccelerationStructure::update(gfx::SceneDescription const& scene) {
    // If the previous acceleration structure update is marked as done, we can swap the
    // acceleration structures.
    // Note that the new BLAS might still be out of date, we still need to check this.
    if (update_done) {
        update_done = false;
        std::swap(accel_structure, async_update_accel_structure);
    }

    // Check if our current acceleration structure contains all unique meshes in the scene.
    // If not, we need to queue a BLAS update with the new mesh list.
    auto unique_meshes = find_unique_meshes(scene);
    // Simple size test already eliminates a bunch of cases.
    bool must_rebuild = false;
    if (unique_meshes.size() != accel_structure->meshes.size()) { must_rebuild = true; }
    else {
        for (auto const &a: unique_meshes) {
            if (std::find(accel_structure->meshes.begin(), accel_structure->meshes.end(), a) ==
                accel_structure->meshes.end()) {
                must_rebuild = true;
            }
        }
    }

    // At least one mesh wasn't found, we need to enqueue a BLAS update task.
    if (must_rebuild) {
        // For now, we won't do this asynchronously, so we can make sure everything works.
        // If this is a performance problem later we'll switch to async
        auto &builder = accel_structure->builder;
        builder.clear_meshes();
        // Note that we know each of these meshes are ready
        for (Handle<gfx::Mesh> handle: unique_meshes) {
            gfx::Mesh const &mesh = *assets::get(handle);
            uint32_t const index = builder.add_mesh(ph::AccelerationStructureMesh{
                    .vertices = mesh.vertices,
                    .vertex_format = VK_FORMAT_R32G32B32_SFLOAT,
                    .num_vertices = mesh.num_vertices,
                    .indices = mesh.indices,
                    .index_type = VK_INDEX_TYPE_UINT32,
                    .num_indices = mesh.num_indices,
                    .stride = (3 + 3 + 3 + 2) * sizeof(float)
            });
            accel_structure->mesh_blas_indices[handle] = index;
        }
        accel_structure->meshes = unique_meshes;
        builder.build_blas_only(0); // Executes on main thread here.
    }

    // While the BLAS is potentially updating on another thread, we continue using the old acceleration structure for now.
    // This does mean that instead of using the meshes we found in the scene as mesh list, we need to use the AS mesh list here, so we don't
    // reference meshes that don't exist in the AS.
    // Note that it's no problem that there might still be meshes in this old AS that aren't being rendered anymore.
    // They will be removed once the update is done, so the overhead is only there for a couple frames at most.

    auto const& meshes = accel_structure->meshes;
    // Clear old instance data, fill with new instances
    auto& builder = accel_structure->builder;
    builder.clear_instances();
    auto const& draws = scene.get_draws();
    auto const& transforms = scene.get_draw_transforms();
    for (uint32_t draw_index = 0; draw_index < draws.size(); ++draw_index) {
        Handle<gfx::Mesh> mesh = draws[draw_index].mesh;
        if (!assets::is_ready(mesh)) continue;
        // Acceleration structure transforms are row-major
        glm::mat4 transform = glm::transpose(transforms[draw_index]);
        ph::AccelerationStructureInstance instance{};
        // TODO: Properly set cull mask based on occluder flag
        instance.cull_mask = 0xFF;
        instance.hit_group_index = 0;
        instance.custom_id = draw_index;
        instance.mesh_index = accel_structure->mesh_blas_indices[mesh];
        std::memcpy(instance.transform, &transform, 12 * sizeof(float));
        builder.add_instance(instance);
    }
    if (!unique_meshes.empty()) {
        builder.build_tlas_only(0); // running on main thread here
    }
}

ph::AccelerationStructure const& SceneAccelerationStructure::get_acceleration_structure() const {
    return accel_structure->as;
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