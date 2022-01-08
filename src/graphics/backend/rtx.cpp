#include <andromeda/graphics/backend/rtx.hpp>
#include <andromeda/util/memory.hpp>

namespace andromeda::gfx::backend {

#define PH_RTX_CALL(func, ...) ctx.rtx_fun._##func(__VA_ARGS__)

SceneAccelerationStructure::SceneAccelerationStructure(gfx::Context& ctx) : ctx(ctx) {
    // Create semaphores for each TLAS.
    for (auto& tlas : top_level) {
        tlas.build_completed = ctx.create_semaphore();
        tlas.buffer.type = ph::BufferType::AccelerationStructure; // Make sure to set buffer properly so ensure_buffer_size() works.
    }

    // Set buffer types so these buffers will be created with the correct type in ensure_buffer_size()
    instance_scratch_buffer.type = ph::BufferType::TransferBuffer;
    instance_buffer.type = ph::BufferType::AccelerationStructureInstance;
    tlas_scratch.type = ph::BufferType::AccelerationStructureScratch;

    instance_upload_semaphore = ctx.create_semaphore();
}

SceneAccelerationStructure::~SceneAccelerationStructure() {
    // Destroy all objects.
    for (auto& tlas : top_level) {
        destroy(tlas);

        if (tlas.compute_cmd.handle() != nullptr) {
            ctx.get_queue(ph::QueueType::Compute)->free_single_time(tlas.compute_cmd, 0);
        }
        if (tlas.transfer_cmd.handle() != nullptr) {
            ctx.get_queue(ph::QueueType::Transfer)->free_single_time(tlas.transfer_cmd, 0);
        }

        ctx.destroy_semaphore(tlas.build_completed);
    }
    ctx.destroy_buffer(instance_scratch_buffer);
    ctx.destroy_buffer(instance_buffer);
    ctx.destroy_buffer(tlas_scratch);
    ctx.destroy_semaphore(instance_upload_semaphore);

    destroy(bottom_level);

    // If there is a BLAS update busy on another thread,
    // queue a task to free that updated BLAS once it's done
    // using a task dependency.
    if (blas_update_task != static_cast<thread::task_id>(-1)) {
        ctx.get_scheduler().schedule([this](auto thread) {
            destroy(updated_blas);
        }, { blas_update_task });
    }
}

void SceneAccelerationStructure::update(gfx::SceneDescription const& scene) {
    // 1) We destroy items in the BLAS destroy queue.
    process_deletion_queue();
    // 2) Swap BLAS if update is done
    if (blas_update_done) {
        // We need to queue deletion for the old BLAS, since it might still be in use.
        queue_delete(bottom_level);
        // Now swap to the new BLAS.
        bottom_level = std::move(updated_blas);
        updated_blas = {};
        // Reset synchronization objects so a new update can be queued properly.
        blas_update_done = false;
        blas_update_task = -1;
    }

    // 3) Queue BLAS update if necessary.
    if (must_update_blas(scene)) {
        rebuild_blas_async(scene);
    }

    // 4) Set current TLAS by incrementing the index once mod N.
    tlas_index = (tlas_index + 1) % top_level.size();
    // 5) Rebuild current TLAS, synchronously, but only if there are meshes (handled inside the function).
    do_tlas_build(scene);
}

[[nodiscard]] VkAccelerationStructureKHR SceneAccelerationStructure::get_acceleration_structure() const {
    return top_level[tlas_index].as.handle;
}

[[nodiscard]] VkSemaphore SceneAccelerationStructure::get_tlas_build_semaphore() const {
    return top_level[tlas_index].build_completed;
}

void SceneAccelerationStructure::destroy(TLAS& tlas) {
    ctx.destroy_buffer(tlas.buffer);
    ctx.destroy_acceleration_structure(tlas.as.handle);
}

void SceneAccelerationStructure::destroy(BLAS& blas) {
    ctx.destroy_buffer(blas.buffer);
    for (AllocatedAS& entry : blas.entries) {
        ctx.destroy_acceleration_structure(entry.handle);
    }
}

void SceneAccelerationStructure::queue_delete(BLAS const& blas) {
    std::lock_guard lock { queue_mutex };
    blas_deletion_queue.push_back(blas);
}

void SceneAccelerationStructure::process_deletion_queue() {
    std::lock_guard lock { queue_mutex };
    for (auto& blas : blas_deletion_queue) {
        destroy(blas);
    }
    blas_deletion_queue.clear();
}

bool SceneAccelerationStructure::must_update_blas(gfx::SceneDescription const& scene) {
    // First check if there is no BLAS update task currently running, and return false if there is.
    if (blas_update_task != static_cast<thread::task_id>(-1)) return false;

    auto scene_meshes = find_unique_meshes(scene);
    // Simple size test will already satisfy most cases.
    if (scene_meshes.size() != bottom_level.mesh_indices.size()) return true;
    // If the sizes do match, check each mesh in scene_meshes against the BLAS mesh map.
    for (Handle<gfx::Mesh> mesh : scene_meshes) {
        // If this mesh is not in the BLAS, return true (updated needed).
        if (bottom_level.mesh_indices.find(mesh) == bottom_level.mesh_indices.end()) {
            return true;
        }
    }
    // All meshes found, no update necessary.
    return false;
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

void SceneAccelerationStructure::rebuild_blas_async(gfx::SceneDescription const& scene) {
    // Since another thread will be accessing the scene, we need to store all meshes
    // that will go into the rebuilt BLAS now.
    std::vector<Handle<gfx::Mesh>> scene_meshes = find_unique_meshes(scene);

    blas_update_task = ctx.get_scheduler().schedule([this, scene_meshes](uint32_t const thread) {
        do_blas_rebuild(scene_meshes, thread);
    }, {});
}

namespace impl {

struct BLASEntry {
    VkAccelerationStructureGeometryKHR geometry{};
    VkAccelerationStructureBuildRangeInfoKHR range{};
};

struct BLASBuildInfo {
    VkAccelerationStructureBuildGeometryInfoKHR geometry{};
    VkAccelerationStructureBuildSizesInfoKHR sizes{};

    // Offset of this BLAS entry in the main BLAS buffer.
    VkDeviceSize buffer_offset = 0;
    // Offset of this BLAS entry in the scratch buffer.
    VkDeviceSize scratch_offset = 0;
};

struct BLASBuildResources {
    ph::RawBuffer scratch_buffer {};
    VkDeviceAddress scratch_address {};

    VkQueryPool compacted_size_qp {};
};

// Build a vector of BLAS entries. Assumes each mesh handle in the given meshes array is unique.
// Also updates the index map.
std::vector<BLASEntry> get_blas_entries(gfx::Context& ctx, std::vector<Handle<gfx::Mesh>> const& meshes, std::unordered_map<Handle<gfx::Mesh>, uint32_t>& index_map) {
    std::vector<BLASEntry> entries;
    entries.reserve(meshes.size());

    for (Handle<gfx::Mesh> handle : meshes) {
        gfx::Mesh const& mesh = *assets::get(handle);

        BLASEntry entry{};
        entry.geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        // TODO: Mark meshes as opaque/non-opaque. (Optimal for tracing is opaque).
        entry.geometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR | VK_GEOMETRY_OPAQUE_BIT_KHR;
        entry.geometry.geometry.triangles = VkAccelerationStructureGeometryTrianglesDataKHR {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
            .pNext = nullptr,
            // This format only describes position data
            .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
            .vertexData = ctx.get_device_address(mesh.vertices),
            // TODO: More flexible vertex stride/formats in engine?
            .vertexStride = (3 + 3 + 3 + 2) * sizeof(float),
            .maxVertex = mesh.num_vertices,
            .indexType = VK_INDEX_TYPE_UINT32,
            .indexData = ctx.get_device_address(mesh.indices),
            // Leave transform empty, as transforms are per-instance.
            .transformData = {}
        };
        entry.geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        entry.range = VkAccelerationStructureBuildRangeInfoKHR {
            .primitiveCount = mesh.num_indices / 3,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
        };
        // Store mesh->index mapping
        uint32_t const index = entries.size();
        index_map[handle] = index;
        // Store BLAS entry
        entries.push_back(entry);
    }

    return entries;
}

// From the spec: 'offset is an offset in bytes from the base address of the buffer at which the acceleration structure will be stored, and must be a multiple of 256'
// (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkAccelerationStructureCreateInfoKHR.html)
static constexpr VkDeviceSize as_buffer_alignment = 256;

std::vector<BLASBuildInfo> get_blas_build_infos(gfx::Context& ctx, std::vector<BLASEntry> const& entries) {
    std::vector<BLASBuildInfo> result{};
    result.reserve(entries.size());

    VkDeviceSize buffer_offset = 0;
    VkDeviceSize scratch_offset = 0;

    for (BLASEntry const& entry : entries) {
        BLASBuildInfo info{};
        info.geometry = VkAccelerationStructureBuildGeometryInfoKHR {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .pNext = nullptr,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            // We set these fields later
            .srcAccelerationStructure = nullptr,
            .dstAccelerationStructure = nullptr,
            .geometryCount = 1,
            .pGeometries = &entry.geometry
        };
        // Make sure to set sType before calling vkGetAccelerationStructureBuildSizesKHR
        info.sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        // Fills in the VkAccelerationStructureBuildSizesInfoKHR structure
        PH_RTX_CALL(vkGetAccelerationStructureBuildSizesKHR, ctx.device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info.geometry, &entry.range.primitiveCount, &info.sizes);

        // As a final step we want to align the size of the acceleration structure buffer and scratch buffer to the proper alignment.
        info.sizes.accelerationStructureSize = util::align_size(info.sizes.accelerationStructureSize, as_buffer_alignment);

        VkDeviceSize const scratch_buffer_alignment = ctx.get_physical_device().accel_structure_properties.minAccelerationStructureScratchOffsetAlignment;
        info.sizes.buildScratchSize = util::align_size(info.sizes.buildScratchSize, scratch_buffer_alignment);

        // Store offsets for easier access
        info.buffer_offset = buffer_offset;
        info.scratch_offset = scratch_offset;

        buffer_offset += info.sizes.accelerationStructureSize;
        scratch_offset += info.sizes.buildScratchSize;

        result.push_back(info);
    }

    return result;
}

// Returns the total amount of scratch memory needed for this full BLAS build.
// We do this so we can suballocate one large scratch buffer.
VkDeviceSize total_scratch_memory(std::vector<impl::BLASBuildInfo> const& build_infos) {
    VkDeviceSize size = 0;
    for (auto const& info : build_infos) {
        size += info.sizes.buildScratchSize;
    }
    return size;
}

// Get the total required memory for a BLAS. This includes extra bytes for alignment.
VkDeviceSize total_blas_memory(std::vector<impl::BLASBuildInfo> const& build_infos) {
    VkDeviceSize size = 0;
    for (auto const& info : build_infos) {
        size += info.sizes.accelerationStructureSize;
    }
    return size;
}

// Create VkAccelerationStructure handles and buffer.
// Since this creates the acceleration structure, it will also write to the dstAccelerationStructure field in build_infos.
void create_blas(gfx::Context& ctx, SceneAccelerationStructure::BLAS& blas, std::vector<impl::BLASBuildInfo>& build_infos) {
    VkDeviceSize const buffer_size = total_blas_memory(build_infos);
    // Creates a big buffer for all BLAS memory.
    blas.buffer = ctx.create_buffer(ph::BufferType::AccelerationStructure, buffer_size);
    ctx.name_object(blas.buffer.handle, "[Buffer] BLAS memory");
    LOG_FORMAT_NOW(LogLevel::Performance, "RT: Total BLAS size (uncompacted): {:.2f} MiB", util::bytes_to_MiB(buffer_size));
    // One entry for each BuildInfo supplied.
    blas.entries.reserve(build_infos.size());
    // Now suballocate this buffer based on offsets computed earlier
    for (impl::BLASBuildInfo& info : build_infos) {
        SceneAccelerationStructure::AllocatedAS entry{};
        entry.memory = blas.buffer.slice(info.buffer_offset, info.sizes.accelerationStructureSize);

        // Create the actual acceleration structure
        VkAccelerationStructureCreateInfoKHR create_info {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .createFlags = {},
            .buffer = blas.buffer.handle,
            .offset = entry.memory.offset,
            .size = entry.memory.range,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .deviceAddress = {}
        };
        PH_RTX_CALL(vkCreateAccelerationStructureKHR, ctx.device(), &create_info, nullptr, &entry.handle);
        // Store handle in build info.
        info.geometry.dstAccelerationStructure = entry.handle;

        blas.entries.push_back(entry);
    }
}

// Create scratch buffer and query pool
BLASBuildResources create_build_resources(gfx::Context& ctx, std::vector<BLASBuildInfo> const& build_infos, uint32_t const thread) {
    BLASBuildResources resources {};
    VkDeviceSize const scratch_size = total_scratch_memory(build_infos);
    resources.scratch_buffer = ctx.create_buffer(ph::BufferType::AccelerationStructureScratch, scratch_size);
    ctx.name_object(resources.scratch_buffer.handle, "[Buffer] BLAS Scratch");
    resources.scratch_address = ctx.get_device_address(resources.scratch_buffer);
    resources.compacted_size_qp = ctx.create_query_pool(VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, build_infos.size());

    LOG_FORMAT_NOW(LogLevel::Performance, "RT: BLAS Scratch memory allocated: {:.2f} MiB", util::bytes_to_MiB(scratch_size));

    return resources;
}

// Assign each BuildInfo a piece of the scratch memory buffer.
void suballocate_scratch_memory(std::vector<BLASBuildInfo>& build_infos, BLASBuildResources const& resources) {
    for (auto& info : build_infos) {
        info.geometry.scratchData.deviceAddress = resources.scratch_address + info.scratch_offset;
    }
}

void build_blas(gfx::Context& ctx, std::vector<BLASEntry> const& entries, std::vector<BLASBuildInfo> const& build_infos, BLASBuildResources& resources, uint32_t const thread) {
    ph::Queue& queue = *ctx.get_queue(ph::QueueType::Compute);
    ph::CommandBuffer cmd = queue.begin_single_time(thread);

    // Build a list of all VkAccelerationStructureKHR handles, so we can do the compacted size query in a single command
    std::vector<VkAccelerationStructureKHR> handles{};

    for (int i = 0; i < entries.size(); ++i) {
        cmd.build_acceleration_structure(build_infos[i].geometry, &entries[i].range);
        handles.push_back(build_infos[i].geometry.dstAccelerationStructure);
        // Note that since we nicely suballocated the scratch memory we don't need barriers between these calls.
    }

    // Submit a query for the compacted size here, so we can wait for it in compact_blas()
    cmd.reset_query_pool(resources.compacted_size_qp, 0, entries.size());
    cmd.write_acceleration_structure_properties(handles, VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, resources.compacted_size_qp, 0);

    VkFence fence = ctx.create_fence();
    queue.end_single_time(cmd, fence);
    ctx.wait_for_fence(fence);
    ctx.destroy_fence(fence);
    queue.free_single_time(cmd, thread);
}

void compact_blas(gfx::Context& ctx, SceneAccelerationStructure::BLAS& blas, std::vector<BLASBuildInfo> const& build_infos, BLASBuildResources& resources, uint32_t const thread) {
    uint32_t const num_blas = build_infos.size();

    // Read back the query results.
    std::vector<uint32_t> compacted_sizes;
    compacted_sizes.resize(num_blas);
    vkGetQueryPoolResults(ctx.device(), resources.compacted_size_qp,
                          0, num_blas, num_blas * sizeof(uint32_t),
                          compacted_sizes.data(), sizeof(uint32_t), VK_QUERY_RESULT_WAIT_BIT);

    // Align each compacted size
    // Also compute offsets into main buffer for each AS, and also compute total compacted size
    VkDeviceSize total_size = 0;
    std::vector<VkDeviceSize> offsets{};
    offsets.resize(num_blas);
    for (size_t i = 0; i < num_blas; ++i) {
        compacted_sizes[i] = util::align_size(compacted_sizes[i], (uint32_t)as_buffer_alignment);
        offsets[i] = total_size;
        total_size += compacted_sizes[i];
    }

    LOG_FORMAT_NOW(LogLevel::Performance, "RT: Total BLAS size (compacted): {:.2f} MiB ({:.1f}% of original).",
                   util::bytes_to_MiB(total_size), ((float)total_size / blas.buffer.size) * 100.0f);

    std::vector<SceneAccelerationStructure::AllocatedAS> compacted_as{};
    compacted_as.reserve(num_blas);
    ph::RawBuffer compacted_buffer = ctx.create_buffer(ph::BufferType::AccelerationStructure, total_size);

    // Now that we have the buffer we can create the final compacted acceleration structures.
    for (size_t i = 0; i < num_blas; ++i) {
        SceneAccelerationStructure::AllocatedAS as{};
        as.memory = compacted_buffer.slice(offsets[i], compacted_sizes[i]);
        VkAccelerationStructureCreateInfoKHR create_info{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .createFlags = {},
            .buffer = compacted_buffer.handle,
            .offset = as.memory.offset,
            .size = as.memory.range,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .deviceAddress = {}
        };
        PH_RTX_CALL(vkCreateAccelerationStructureKHR, ctx.device(), &create_info, nullptr, &as.handle);
        ctx.name_object(as.handle, fmt::vformat("BLAS [{}]", fmt::make_format_args(i)));
        compacted_as.push_back(as);
    }

    ph::Queue& queue = *ctx.get_queue(ph::QueueType::Compute);
    ph::CommandBuffer cmd = queue.begin_single_time(thread);

    // Now that we created the compacted acceleration structures we can record the commands for compaction
    for (size_t i = 0; i < num_blas; ++i) {
        cmd.compact_acceleration_structure(blas.entries[i].handle, compacted_as[i].handle);
    }

    VkFence fence = ctx.create_fence();
    queue.end_single_time(cmd, fence);
    ctx.wait_for_fence(fence);
    ctx.destroy_fence(fence);
    queue.free_single_time(cmd, thread);

    // Compaction is now complete, we need to destroy the old acceleration structures and swap them with the new ones.
    ctx.destroy_buffer(blas.buffer);
    for (auto& as : blas.entries) {
        ctx.destroy_acceleration_structure(as.handle);
    }
    blas.buffer = compacted_buffer;
    blas.entries = compacted_as;
}

void free_build_resources(gfx::Context& ctx, BLASBuildResources& resources) {
    ctx.destroy_buffer(resources.scratch_buffer);
    ctx.destroy_query_pool(resources.compacted_size_qp);
}

} // namespace impl


void SceneAccelerationStructure::do_blas_rebuild(std::vector<Handle<gfx::Mesh>> const& meshes, const uint32_t thread) {
    LOG_WRITE_NOW(LogLevel::Performance, "RT: BLAS rebuild triggered.");
    // This is the BLAS we will be putting our result in. We can assume that this is an empty BLAS.
    BLAS& blas = updated_blas;
    // 1) For each mesh we will get a VkAccelerationStructureGeometryKHR and VkAccelerationStructureBuildRangeInfoKHR,
    //    and build a mapping from mesh handle -> index into the BLASEntry vector (used for TLAS building).
    std::vector<impl::BLASEntry> entries = impl::get_blas_entries(ctx, meshes, blas.mesh_indices);
    // 2) Pack these entries into a VkAccelerationStructureBuildGeometryInfoKHR and VkAccelerationStructureBuildSizesInfoKHR.
    std::vector<impl::BLASBuildInfo> build_infos = impl::get_blas_build_infos(ctx, entries);
    // 3) Create the actual (uncompacted) acceleration structures.
    impl::create_blas(ctx, blas, build_infos);
    // 4) Build the BLAS. To do this we will first need to create all build resources and distribute scratch memory.
    impl::BLASBuildResources resources = impl::create_build_resources(ctx, build_infos, thread);
    impl::suballocate_scratch_memory(build_infos, resources);
    // This will create a command buffer with build commands, submit it and wait on it.
    impl::build_blas(ctx, entries, build_infos, resources, thread);
    // 5) Compact the BLAS.
    impl::compact_blas(ctx, blas, build_infos, resources, thread);
    // 6) Free up resources
    impl::free_build_resources(ctx, resources);
    // Finally, we signal to the main thread that the BLAS build is complete
    blas_update_done = true;
    LOG_WRITE_NOW(LogLevel::Performance, "RT: BLAS rebuild completed.");
}

namespace impl {

struct TLASInstances {
    VkAccelerationStructureGeometryKHR instances{};
    VkAccelerationStructureBuildRangeInfoKHR range{};
};

struct TLASBuildInfo {
    VkAccelerationStructureBuildGeometryInfoKHR geometry{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    VkAccelerationStructureBuildSizesInfoKHR size_info{ .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
};

// return true if the buffer was resized
bool ensure_buffer_size(gfx::Context& ctx, ph::RawBuffer& buffer, VkDeviceSize size, std::string_view name, uint32_t const index) {
    if (buffer.size < size) {
        ctx.destroy_buffer(buffer);
        buffer = ctx.create_buffer(buffer.type, size);
        ctx.name_object(buffer.handle, fmt::vformat("[Buffer] TLAS [F{}] {} buffer", fmt::make_format_args(index, name)));
        LOG_FORMAT_NOW(LogLevel::Performance, "RT: TLAS {} buffer resized to {:.2f} KiB", name, util::bytes_to_KiB(size));
        return true;
    }
    return false;
}

std::vector<VkAccelerationStructureInstanceKHR> get_instance_data(gfx::Context& ctx, gfx::SceneDescription const& scene, SceneAccelerationStructure::BLAS const& blas) {
    std::vector<VkAccelerationStructureInstanceKHR> result {};
    auto draws = scene.get_draws();
    auto transforms = scene.get_draw_transforms();
    result.reserve(draws.size()); // this might be slightly too large due to assets that aren't ready, or the BLAS not being updated yet.
    for (uint32_t i = 0; i < draws.size(); ++i) {
        auto const& draw = draws[i];
        // Skip instances that are not in the BLAS
        auto it = blas.mesh_indices.find(draw.mesh);
        if (it == blas.mesh_indices.end()) continue;

        uint32_t const index = it->second;

        VkAccelerationStructureInstanceKHR info{};

        // Get device address of BLAS
        VkAccelerationStructureDeviceAddressInfoKHR blas_address_info{};
        blas_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        blas_address_info.accelerationStructure = blas.entries[index].handle;
        VkDeviceAddress blas_address = PH_RTX_CALL(vkGetAccelerationStructureDeviceAddressKHR, ctx.device(), &blas_address_info);

        info.accelerationStructureReference = blas_address;
        info.instanceCustomIndex = i;
        info.instanceShaderBindingTableRecordOffset = 0; // TODO: hit group index. -> This will probably go in material settings
        info.mask = 0xFF; // TODO: various culling masks.
        info.flags = {};
        // 4x3 transposed transform matrix
        glm::mat4 transform = glm::transpose(transforms[i]);
        std::memcpy(&info.transform, &transform, 12 * sizeof(float));

        result.push_back(info);
    }
    return result;
}

// Returns a command buffer from the transfer queue that must be freed when the TLAS build is completed
ph::CommandBuffer create_instance_buffer(gfx::Context& ctx, ph::RawBuffer& scratch, ph::RawBuffer& instances,
                                         std::vector<VkAccelerationStructureInstanceKHR> const& data, VkSemaphore semaphore, uint32_t const index) {
    VkDeviceSize const size = util::memsize(data);
    ensure_buffer_size(ctx, scratch, size, "scratch", index);
    ensure_buffer_size(ctx, instances, size, "instance", index);

    std::byte* memory = ctx.map_memory(scratch);
    std::memcpy(memory, data.data(), size);
    ctx.unmap_memory(scratch);

    ph::Queue& queue = *ctx.get_queue(ph::QueueType::Transfer);
    ph::CommandBuffer cmd = queue.begin_single_time(0); // TLAS build is on main thread.

    cmd.copy_buffer(scratch, instances);
    queue.end_single_time(cmd, nullptr, {}, nullptr, semaphore);

    return cmd;
}

TLASInstances get_instances(gfx::Context& ctx, ph::RawBuffer& instances, uint32_t const instance_count) {
    TLASInstances info {};
    VkAccelerationStructureGeometryInstancesDataKHR instances_vk{};
    instances_vk.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instances_vk.arrayOfPointers = false;
    instances_vk.data.deviceAddress = ctx.get_device_address(instances);

    info.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    info.instances.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    info.instances.geometry.instances = instances_vk;
    // The amount of primitives is the amount of instances. We can leave all other values at zero.
    info.range.primitiveCount = instance_count;
    return info;
}

TLASBuildInfo get_tlas_build_info(ph::Context& ctx, TLASInstances const& tlas) {
    TLASBuildInfo info{};
    info.geometry.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    info.geometry.srcAccelerationStructure = nullptr;
    info.geometry.dstAccelerationStructure = nullptr; // We fill this field later
    info.geometry.geometryCount = 1;
    info.geometry.pGeometries = &tlas.instances;
    info.geometry.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    // Get size info for TLAS
    uint32_t const num_instances = tlas.range.primitiveCount;
    PH_RTX_CALL(vkGetAccelerationStructureBuildSizesKHR, ctx.device(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &info.geometry, &num_instances, &info.size_info);
    return info;
}

// returns true if either one of scratch or memory was reallocated.
bool allocate_tlas_memory(gfx::Context& ctx, ph::RawBuffer& memory, ph::RawBuffer& scratch, TLASBuildInfo const& build_info, uint32_t const index) {
    bool b1 = ensure_buffer_size(ctx, memory, build_info.size_info.accelerationStructureSize, "main memory", index);
    bool b2 = ensure_buffer_size(ctx, scratch, build_info.size_info.buildScratchSize, "build scratch", index);
    return b1 || b2;
}

void create_tlas(gfx::Context& ctx, SceneAccelerationStructure::TLAS& tlas, TLASBuildInfo const& build_info, uint32_t const index) {
    VkAccelerationStructureCreateInfoKHR info {};
    info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    info.size = build_info.size_info.accelerationStructureSize;
    info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    info.buffer = tlas.buffer.handle;
    PH_RTX_CALL(vkCreateAccelerationStructureKHR, ctx.device(), &info, nullptr, &tlas.as.handle);
    ctx.name_object(tlas.as.handle, fmt::vformat("TLAS [F{}]", fmt::make_format_args(index)));
    tlas.as.memory = tlas.buffer.slice(0, build_info.size_info.accelerationStructureSize);
}

// Returns a command buffer from the compute queue, must be freed before building this frame's TLAS for the next time.
ph::CommandBuffer build_tlas(gfx::Context& ctx, SceneAccelerationStructure::TLAS& tlas, TLASInstances const& instances, TLASBuildInfo& build_info, VkSemaphore upload, ph::RawBuffer const& scratch) {
    ph::Queue& queue = *ctx.get_queue(ph::QueueType::Compute);
    ph::CommandBuffer cmd = queue.begin_single_time(0);
    VkDeviceAddress const scratch_address = ctx.get_device_address(scratch);
    // Update build info structure with proper dstAccelerationStructure and scratch buffer device addresses
    build_info.geometry.dstAccelerationStructure = tlas.as.handle;
    build_info.geometry.scratchData.deviceAddress = scratch_address;
    cmd.build_acceleration_structure(build_info.geometry, &instances.range);
    // Submit command buffer, waiting on the transfer semaphore and signaling the completion semaphore
    queue.end_single_time(cmd, nullptr, ph::PipelineStage::AccelerationStructureBuild, upload, tlas.build_completed);
    return cmd;
}

} // namespace impl

void SceneAccelerationStructure::do_tlas_build(gfx::SceneDescription const& scene) {
    TLAS& tlas = top_level[tlas_index];

    if (tlas.compute_cmd.handle() != nullptr) {
        ctx.get_queue(ph::QueueType::Compute)->free_single_time(tlas.compute_cmd, 0);
    }
    if (tlas.transfer_cmd.handle() != nullptr) {
        ctx.get_queue(ph::QueueType::Transfer)->free_single_time(tlas.transfer_cmd, 0);
    }

    // 1) Pack scene structure into instance data.
    std::vector<VkAccelerationStructureInstanceKHR> instances = impl::get_instance_data(ctx, scene, bottom_level);
    // If there are no instances, this is the moment to abort.
    if (instances.empty()) return;
    // 2) Upload instance data to buffer
    tlas.transfer_cmd = impl::create_instance_buffer(ctx, instance_scratch_buffer, instance_buffer, instances, instance_upload_semaphore, tlas_index);
    // 3) Create structure to point build info to the instance buffer
    impl::TLASInstances tlas_instances = impl::get_instances(ctx, instance_buffer, instances.size());
    // 4) Get build info
    impl::TLASBuildInfo build_info = impl::get_tlas_build_info(ctx, tlas_instances);
    // 5) Allocate enough scratch and buffer space
    bool const reallocate = impl::allocate_tlas_memory(ctx, tlas.buffer, tlas_scratch, build_info, tlas_index);
    // 6) Create the TLAS handle (only if any of the buffers was reallocated).
    if (reallocate) {
        // Destroy old TLAS and create a new one.
        if (tlas.as.handle) PH_RTX_CALL(vkDestroyAccelerationStructureKHR, ctx.device(), tlas.as.handle, nullptr);
        impl::create_tlas(ctx, tlas, build_info, tlas_index);
    }
    // 7) Build the TLAS, this does not block, but instead signals the TLAS's semaphore when done.
    //    The returned command buffer must be freed the next time this TLAS is used.
    tlas.compute_cmd = impl::build_tlas(ctx, tlas, tlas_instances, build_info, instance_upload_semaphore, tlas_scratch);
}

} // namespace gfx::backend