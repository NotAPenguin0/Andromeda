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

    ~SceneAccelerationStructure();

    /**
     * @brief Updates the acceleration structure for this frame. TLAS updates are instant, while a BLAS update may have a couple frames delay
     *        as this is done asynchronously.
     * @param scene Reference to the scene information.
     */
    void update(gfx::SceneDescription const& scene);

    /**
     * @brief Gets the acceleration structure to use for rendering. For correct results, update() must be called first.
     *        Note that the acceleration structure build might still be in progress. For a correct result, you MUST
     *        submit any command buffer that uses this acceleration structure with the semaphore obtained from
     *        get_tlas_build_semaphore()
     * @return Handle to the top-level acceleration structure.
     */
    [[nodiscard]] VkAccelerationStructureKHR get_acceleration_structure() const;

    /**
     * @brief This semaphore must be waited on before using a TLAS.
     * @return Handle to a VkSemaphore to wait on.
     */
    [[nodiscard]] VkSemaphore get_tlas_build_semaphore() const;

private:
    gfx::Context& ctx;

    // Represents a single BLAS or TLAS allocated from a buffer.
    struct AllocatedAS {
        VkAccelerationStructureKHR handle {};
        ph::BufferSlice memory {};
    };

    // Stores the entire top-level acceleration structure.
    struct TLAS {
        // The memory slice of this AS will simply be a part of (or the entire) buffer member below.
        AllocatedAS as {};
        // We will use one buffer for each TLAS.
        // This buffer will be re-used for each TLAS update.
        ph::RawBuffer buffer {};
        // Used to synchronize access from the main render passes.
        VkSemaphore build_completed {};
    };

    // Stores the entire bottom-level acceleration structure.
    struct BLAS {
        // Each entry has a BufferSlice into the buffer below.
        std::vector<AllocatedAS> entries {};
        // One buffer for all entries in the BLAS.
        ph::RawBuffer buffer {};
    };

    // This buffer may be reused even if the GPU is busy using the previous acceleration structure
    ph::RawBuffer instance_scratch_buffer {};

    // Each in-flight frame needs one TLAS, since we're continuously updating these.
    std::array<TLAS, 2> top_level {};
    // Index in the array of TLAS's. Indicates the TLAS used for this frame.
    uint32_t tlas_index = 0;

    // We can share a single BLAS across frames because updates will be done asynchronously.
    BLAS bottom_level {};

    // Signals that the BLAS update is completed
    std::atomic<bool> blas_update_done = false;

    // These must be deleted before checking blas_update_done and reassigning the new BLAS.
    std::vector<BLAS> blas_deletion_queue {};

    /**
    * @brief Find all unique meshes that are also ready and collect them into a vector.
    * @param scene Scene data.
    * @return List of unique mesh handles that are all marked as ready by the asset system.
    */
    std::vector<Handle<gfx::Mesh>> find_unique_meshes(gfx::SceneDescription const& scene);
};

}