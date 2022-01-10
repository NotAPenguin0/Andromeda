#include <andromeda/graphics/performance_counters.hpp>

namespace andromeda::gfx {

void StatTracker::set_interval(uint32_t i) {
    interval = i;
}

void StatTracker::new_frame() {
    frame += 1;
    frame = frame % interval;
    // Reset stats if we will update this frame.
    if (must_update()) stats = {};
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

bool StatTracker::must_update() {
    return frame == 0;
}

}