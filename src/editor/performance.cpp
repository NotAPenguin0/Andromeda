#include <andromeda/editor/performance.hpp>

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
            std::string const& text = fmt::format(FMT_STRING("Frame time: {:.2f} ms ({} fps)."), average_frametime, fps);
            ImGui::TextUnformatted(text.c_str());
        }
        ImGui::End();
    }
}

bool& PerformanceDisplay::is_visible() {
    return visible;
}

}