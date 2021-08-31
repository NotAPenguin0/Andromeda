#pragma once

#include <andromeda/graphics/imgui.hpp>

#include <phobos/image.hpp>

namespace andromeda::editor {

/**
 * @brief Displays an image.
 * @param view The image to display.
 * @param size The size of the final image widget. If a component of this is zero, the corresponding component
 *			   of the image's size will be used.
*/
void display_image(ph::ImageView view, ImVec2 size = ImVec2(0, 0));

/**
 * @brief Makes the next window's content size match the image size.
 *		  combine this with no padding to make the image fill the entire window.
 * @param view The image to 
*/
void size_next_window_to_image(ph::ImageView view);

} // namespace andromeda::image