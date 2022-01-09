#pragma once

#include <andromeda/graphics/imgui.hpp>

namespace andromeda::editor {

class PerformanceDisplay {
public:
    void display(gfx::Context& ctx);

    bool& is_visible();
private:
    bool visible = true;

    float average_frametime = 0.0f;
};

}