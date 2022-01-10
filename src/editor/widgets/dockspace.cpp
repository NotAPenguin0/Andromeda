#include <andromeda/editor/widgets/dockspace.hpp>

namespace andromeda::editor {

DockSpace::DockSpace(const char* name, ImGuiViewport* viewport, ImGuiWindowFlags flags) {
    // Setup viewport for dockspace window
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    // A dockspace doesn't need rounding, since it won't be visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    // Add all flags required to make the dockspace window not a "real" window.
    flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    // Pass nullptr for the pOpen parameter to make sure no close button is shown.
    ImGui::Begin(name, nullptr, flags);
    ImGui::PopStyleVar(3);

    // Mark this window as a dockspace
    ImGuiID id = ImGui::GetID(name);
    ImGui::DockSpace(id);
}

DockSpace::~DockSpace() {
    // Matching for ImGui::Begin() in constructor
    ImGui::End();
}

} // namespace andromeda::editor