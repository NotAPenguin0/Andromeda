#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>
#include <phobos/acceleration_structure.hpp>

namespace andromeda::gfx::backend {

/**
 * @class SceneAccelerationStructure
 * @brief Can manage and update the acceleration structure used to trace the scene.
 */
class SceneAccelerationStructure {
public:
    /**
     * @brief Create the acceleration structure manager.
     * @param ctx Reference to the graphics context.
     */
    SceneAccelerationStructure(gfx::Context& ctx);

    /**
     * @brief Updates the acceleration structure for this frame. TLAS updates are instant, while a BLAS update may have a couple frames delay
     *        as this is done asynchronously.
     * @param scene Reference to the scene information.
     */
    void update(gfx::SceneDescription const& scene);

    /**
     * @brief Gets the acceleration structure to use for rendering. For correct results, update() must be called first.
     * @return Const reference to a valid acceleration structure.
     */
    ph::AccelerationStructure const& get_acceleration_structure() const;

private:
    gfx::Context& ctx;

    struct AccelerationStructure {
        explicit inline AccelerationStructure(gfx::Context& ctx) : builder(ph::AccelerationStructureBuilder::create(ctx)) {}

        /**
         * @brief All unique meshes stored inside as BLAS entries.
         */
        std::vector<Handle<gfx::Mesh>> meshes;

        /**
         * @brief Handle to the actual acceleration structure (TLAS + BLAS)
         */
        ph::AccelerationStructure as;

        /**
         * @brief AS builder. We need to keep this around to be able to update acceleration structures.
         */
        ph::AccelerationStructureBuilder builder;

        /**
         * @brief During BLAS build, this will store a mapping from mesh index to
         */
        std::unordered_map<Handle<gfx::Mesh>, uint32_t> mesh_blas_indices;
    };

    /**
     * @brief This is the main acceleration structure that will be returned by get_acceleration_structure()
     */
    std::unique_ptr<AccelerationStructure> accel_structure;

    /**
     * @brief When a BLAS needs to be updated, the update will be done on this acceleration structure.
     */
    std::unique_ptr<AccelerationStructure> async_update_accel_structure;

    /**
     * @brief If two or more acceleration structure update tasks are somehow active at the same time,
     *        use this task ID to set the dependency chain correctly.
     */
    thread::task_id update_task = -1;

    /**
     * @brief Signals that the BLAS update is done and accel_structure and async_accel_structure may be swapped.
     */
     std::atomic<bool> update_done = false;

     /**
      * @brief Find all unique meshes that are also ready and collect them into a vector.
      * @param scene Scene data.
      * @return List of unique mesh handles that are all marked as ready by the asset system.
      */
     std::vector<Handle<gfx::Mesh>> find_unique_meshes(gfx::SceneDescription const& scene);
};

}