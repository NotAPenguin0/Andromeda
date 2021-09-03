#include <andromeda/editor/widgets/tooltip.hpp>

#include <andromeda/editor/style.hpp>

namespace andromeda::editor {

void show_tooltip(std::string const& tooltip, float width) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();

        {
            auto wrap = style::ScopedTextWrapModifier(ImGui::GetFontSize() * width);
            ImGui::TextUnformatted(tooltip.c_str());
        }

        ImGui::EndTooltip();
    }
}

}
