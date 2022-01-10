#include <andromeda/editor/performance.hpp>

#include <andromeda/graphics/performance_counters.hpp>

namespace andromeda::editor {

void PerformanceDisplay::display(gfx::Context& ctx) {
    // Update moving average
    float const a = 0.98f;
    float const dt = ctx.delta_time() * 1000.0f; // seconds to milliseconds.
    average_frametime = average_frametime * a + (1.0f - a) * dt;
    // Display
    if (visible) {
        if (ImGui::Begin("Performance##widget", &visible)) {
            uint32_t const fps = static_cast<uint32_t>(std::floor(1000.0f / average_frametime));
            std::string const frametime_text = fmt::format(FMT_STRING("Frame time: {:.2f} ms ({} fps).\n"), average_frametime, fps);
            ImGui::TextUnformatted(frametime_text.c_str());

            // Display renderer stats from tracker
            ImGui::Separator();
            gfx::PerformanceCounters const& stats = gfx::StatTracker::get();
            std::string const geometry_text = fmt::format(FMT_STRING("vertex count: {}\ntriangle count: {}\n"), stats.vertices, stats.triangles);
            ImGui::TextUnformatted(geometry_text.c_str());
            std::string const commands_text = fmt::format(FMT_STRING("drawcalls: {}\ndispatches: {}"), stats.drawcalls, stats.dispatches);
            ImGui::TextUnformatted(commands_text.c_str());
        }
        ImGui::End();
    }
}

bool& PerformanceDisplay::is_visible() {
    return visible;
}

}