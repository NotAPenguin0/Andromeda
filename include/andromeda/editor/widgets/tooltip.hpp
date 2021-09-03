#pragma once

#include <andromeda/graphics/imgui.hpp>

namespace andromeda::editor {

/**
 * @brief Show a tooltip on hover. This will display a small (?) on the same line as the current element,
 *        and, when hovering that will show the tooltip string.
 * @param tooltip Tooltip string to show.
 * @param width Width of the tooltip popup. Defaults to 35.0.
*/
void show_tooltip(std::string const& tooltip, float width = 35.0f);

}