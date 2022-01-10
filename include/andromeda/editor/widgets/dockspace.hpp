#pragma once

#include <andromeda/graphics/imgui.hpp>

namespace andromeda::editor {

/**
 * @class DockSpace 
 * @brief Defines a docking area for windows. Automatically calls Begin() and End() on
 *		  construction and destruction.
*/
class DockSpace {
public:
    /**
     * @brief Create the dockspace
     * @param name Unique name for the dockspace
     * @param viewport Pointer to the viewport this dockspace will occupy.
     * @param flags Styling flags for the dockspace window.
    */
    DockSpace(const char* name, ImGuiViewport* viewport, ImGuiWindowFlags flags);

    ~DockSpace();
};

} // namespace andromeda::editor