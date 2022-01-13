#include <andromeda/graphics/performance_counters.hpp>
#include <andromeda/util/memory.hpp>

namespace andromeda::gfx {

void StatTracker::initialize(gfx::Context& ctx) {
    VkQueryPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = {};
    info.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    info.queryCount = 1;
    info.pipelineStatistics =
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT
        | VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
    vkCreateQueryPool(ctx.device(), &info, nullptr, &query_pool);
}

void StatTracker::shutdown(gfx::Context& ctx) {
    ctx.destroy_query_pool(query_pool);
}

void StatTracker::set_interval(uint32_t i) {
    interval = i;
}

void StatTracker::new_frame(gfx::Context& ctx) {
    frame += 1;
    frame = frame % interval;
    // Reset stats if we will update this frame.
    if (must_update()) stats = {};
    // Check if query pool results are available
    if (vkGetQueryPoolResults(ctx.device(), query_pool, 0, 1, sizeof(uint32_t) * queries.size(), queries.data(), sizeof(uint32_t), {}) != VK_NOT_READY) {
        // Read back data
        vkGetQueryPoolResults(ctx.device(), query_pool, 0, 1, sizeof(uint32_t) * queries.size(), queries.data(), sizeof(uint32_t), VK_QUERY_RESULT_WAIT_BIT);
        stats.vertex_invocations = queries[0];
        stats.fragment_invocations = queries[1];
        stats.compute_invocations = queries[2];
    }
}

gfx::PerformanceCounters const& StatTracker::get() {
    return stats;
}

void StatTracker::draw(ph::CommandBuffer& cmd, uint32_t num_vertices, uint32_t num_instances, uint32_t first_vertex, uint32_t first_instance) {
    if (must_update()) {
        std::lock_guard lock(mutex);
        stats.vertices += num_vertices * num_instances;
        stats.triangles += (num_vertices / 3) * num_instances;
        stats.drawcalls += 1;
    }
    cmd.draw(num_vertices, num_instances, first_vertex, first_instance);
}

void StatTracker::draw_indexed(ph::CommandBuffer& cmd, uint32_t num_indices, uint32_t num_vertices, uint32_t num_instances, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) {
    if (must_update()) {
        std::lock_guard lock(mutex);
        stats.vertices += num_vertices * num_instances;
        stats.triangles += (num_indices / 3) * num_instances;
        stats.drawcalls += 1;
    }
    cmd.draw_indexed(num_indices, num_instances, first_index, vertex_offset, first_instance);
}

void StatTracker::dispatch(ph::CommandBuffer& cmd, uint32_t x, uint32_t y, uint32_t z) {
    if (must_update()) {
        std::lock_guard lock(mutex);
        stats.dispatches += 1;
    }
    cmd.dispatch(x, y, z);
}

void StatTracker::begin_query(ph::CommandBuffer& cmd) {
    if (must_update()) {
        vkCmdResetQueryPool(cmd.handle(), query_pool, 0, 1);
        vkCmdBeginQuery(cmd.handle(), query_pool, 0, {});
    }
}

void StatTracker::end_query(ph::CommandBuffer& cmd) {
    if (must_update()) {
        vkCmdEndQuery(cmd.handle(), query_pool, 0);
    }
}

bool StatTracker::must_update() {
    return frame == 0;
}

}