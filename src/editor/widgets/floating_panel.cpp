#include <andromeda/editor/widgets/floating_panel.hpp>

namespace andromeda::editor {

FloatingPanel::FloatingPanel(std::string const& name, ImVec2 position, ImVec2 size, float alpha)
    : padding(ImGuiStyleVar_WindowPadding, style::default_style().WindowPadding),
      bg_color(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)) {

    ImVec2 parent_position = ImGui::GetWindowPos();
    position.x += parent_position.x;
    position.y += parent_position.y;

    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowBgAlpha(alpha);

    ImGui::SetCursorPos({0.0f, 0.0f});

    // Don't allow resizing or moving, hide the title bar and disallow collapsing.
    // We add the ChildWindow flag to make sure it doesn't lose focus and hide when clicking
    // anywhere outside the panel.
    // AlwaysUseWindowPadding is necessary because by default child windows will ignore padding.
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_ChildWindow
                             | ImGuiWindowFlags_AlwaysUseWindowPadding;

    begin_result = ImGui::BeginChild(name.c_str(), size, true, flags);
}

void FloatingPanel::display(std::function<void()> func) {
    if (begin_result) {
        func();
    }
}

FloatingPanel::~FloatingPanel() {
    ImGui::EndChild();
}

} // namespace andromeda::editor