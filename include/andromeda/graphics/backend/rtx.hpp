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

    // Internal types, we don't really need these in the public API.

    // Represents a single BLAS or TLAS allocated from a buffer.
    struct AllocatedAS {
        VkAccelerationStructureKHR handle{};
        ph::BufferSlice memory{};
    };

    // Stores the entire top-level acceleration structure.
    struct TLAS {
        // The memory slice of this AS will simply be a part of (or the entire) buffer member below.
        AllocatedAS as{};
        // We will use one buffer for each TLAS.
        // This buffer will be re-used for each TLAS update.
        ph::RawBuffer buffer{};
        // Used to synchronize access from the main render passes.
        VkSemaphore build_completed{};

        ph::CommandBuffer transfer_cmd;
        ph::CommandBuffer compute_cmd;
    };

    // Stores the entire bottom-level acceleration structure.
    struct BLAS {
        // Each entry has a BufferSlice into the buffer below.
        std::vector<AllocatedAS> entries{};
        // One buffer for all entries in the BLAS.
        ph::RawBuffer buffer{};
        // Stores a mapping from mesh handle -> index into the BLAS entry vector.
        std::unordered_map<Handle<gfx::Mesh>, uint32_t> mesh_indices;
    };

private:
    gfx::Context& ctx;

    // These buffers may be reused even if the GPU is busy using the previous acceleration structure

    // Upload to instance_buffer
    ph::RawBuffer instance_scratch_buffer{};
    // Instance buffer for TLAS creation.
    ph::RawBuffer instance_buffer{};
    // Signals that the upload to the instance buffer is done.
    VkSemaphore instance_upload_semaphore{};

    // Each in-flight frame needs one TLAS, since we're continuously updating these.
    std::array<TLAS, 2> top_level{};
    // Index in the array of TLAS's. Indicates the TLAS used for this frame.
    uint32_t tlas_index = 0;

    // Scratch memory for TLAS build
    ph::RawBuffer tlas_scratch{};

    // We can share a single BLAS across frames because updates will be done asynchronously.
    BLAS bottom_level{};

    // When a BLAS update is done, the result is stored here.
    BLAS updated_blas{};

    // Signals that the BLAS update is completed
    std::atomic<bool> blas_update_done = false;

    // These must be deleted before checking blas_update_done and reassigning the new BLAS.
    std::vector<BLAS> blas_deletion_queue{};

    // Protects access to the BLAS deletion queue
    std::mutex queue_mutex{};

    // This exists to make sure we don't start two BLAS updates at the same time.
    thread::task_id blas_update_task = -1;

    // Immediately destroy a TLAS
    void destroy(TLAS& tlas);

    // Immediately destroy a BLAS
    void destroy(BLAS& blas);

    // Queue deletion of a BLAS. Always use this instead of calling destroy() directly.
    void queue_delete(BLAS const& blas);

    // Processes the BLAS deletion queue and destroys each element.
    void process_deletion_queue();

    // Returns true if a BLAS update is needed for the current BLAS.
    bool must_update_blas(gfx::SceneDescription const& scene);

    // Find all unique and ready meshes in the scene.
    std::vector<Handle<gfx::Mesh>> find_unique_meshes(gfx::SceneDescription const& scene);

    // Queues an async task for a BLAS update.
    void rebuild_blas_async(gfx::SceneDescription const& scene);

    // Do a BLAS rebuild on a specific thread. This function is synchronous.
    void do_blas_rebuild(std::vector<Handle<gfx::Mesh>> const& meshes, uint32_t const thread);

    void do_tlas_build(gfx::SceneDescription const& scene);
};

}