#include <andromeda/editor/widgets/image.hpp>

namespace andromeda::editor {

void display_image(ph::ImageView view, ImVec2 size) {
    if (size.x == 0) {
        size.x = view.size.width;
    }

    if (size.y == 0) {
        size.y = view.size.height;
    }

    ImTextureID id = gfx::imgui::get_texture_id(view);
    ImGui::Image(id, size);
}

void size_next_window_to_image(ph::ImageView view) {
    ImGui::SetNextWindowContentSize(ImVec2(view.size.width, view.size.height));
}

} // namespace andromeda::editor