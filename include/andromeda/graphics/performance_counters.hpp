#pragma once

#include <cstdint>
#include <optional>
#include <mutex>

#include <phobos/command_buffer.hpp>

#include <andromeda/graphics/context.hpp>

namespace andromeda::gfx {

struct PerformanceCounters {
    // Amount of vkCmdDraw() or vkCmdDrawIndexed() calls.
    uint64_t drawcalls = 0;
    // Amount of vkCmdDispatch() calls.
    uint64_t dispatches = 0;
    // Total number of submitted triangles.
    uint64_t triangles = 0;
    // Total number of submitted vertices.
    uint64_t vertices = 0;
    // Amount of invocations for each shader stage
    uint32_t vertex_invocations = 0;
    uint32_t fragment_invocations = 0;
    uint32_t compute_invocations = 0;
    // Memory in bytes used by raytracing backend (optional).
    std::optional<uint64_t> rtx_memory = std::nullopt;
};

class StatTracker {
public:
    static void initialize(gfx::Context& ctx);
    static void shutdown(gfx::Context& ctx);
    static void set_interval(uint32_t i);
    static void new_frame(gfx::Context& ctx);
    static gfx::PerformanceCounters const& get();

    static void draw(ph::CommandBuffer& cmd, uint32_t num_vertices, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance);
    static void draw_indexed(ph::CommandBuffer& cmd, uint32_t num_indices, uint32_t num_vertices, uint32_t num_instances, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
    static void dispatch(ph::CommandBuffer& cmd, uint32_t x, uint32_t y, uint32_t z);

    static void begin_query(ph::CommandBuffer& cmd);
    static void end_query(ph::CommandBuffer& cmd);

private:
    static inline gfx::PerformanceCounters stats{};
    static inline std::mutex mutex {};
    // Only update every N frames, other frames are just ignored.
    static inline uint32_t interval = 1;
    static inline uint32_t frame = 0;

    static inline VkQueryPool query_pool = nullptr;
    static inline std::array<uint32_t, 3> queries {};

    static bool must_update();
};

#define vkCmdDraw_Tracked(cmd, num_verts, num_instances, first_vertex, first_instance) \
    ::andromeda::gfx::StatTracker::draw(cmd, num_verts, num_instances, first_vertex, first_instance)
// The num_vertices parameter here is submitted only for tracking purposes and won't be used in the drawcall.
#define vkCmdDrawIndexed_Tracked(cmd, num_indices, num_vertices, num_instances, first_index, vertex_offset, first_instance) \
    ::andromeda::gfx::StatTracker::draw_indexed(cmd, num_indices, num_vertices, num_instances, first_index, vertex_offset, first_instance)
#define vkCmdDispatch_Tracked(cmd, x, y, z) \
    ::andromeda::gfx::StatTracker::dispatch(cmd, x, y, z)
}