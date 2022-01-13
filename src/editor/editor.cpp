#include <andromeda/editor/editor.hpp>

#include <andromeda/editor/widgets/dockspace.hpp>
#include <andromeda/editor/widgets/menu_bar.hpp>
#include <andromeda/editor/style.hpp>
#include <andromeda/editor/scene_viewport.hpp>

#include <andromeda/graphics/context.hpp>

#include <string>

using namespace std::literals::string_literals;

namespace andromeda::editor {

Editor::Editor(gfx::Context& ctx, Window& window) : console(ctx, window) {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Add a new font.
    font = io.Fonts->AddFontFromFileTTF("data/fonts/Roboto-Medium.ttf", 15);
    // Now we merge this with the icon font
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig icons_config{};
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("data/fonts/" FONT_ICON_FILE_NAME_FAS, 15, &icons_config, icons_ranges);

    style::set_rounding(5.0f);
    style::color_preset_gray();

    // Re-route logging function to include the console output
    ::andromeda::impl::_global_log_pointer->set_output([this](LogLevel level, std::string_view str) {
        // We still display output to stdout, but also log into the console.
        Log::stdout_log_func(level, str);
        console.write(level, str);
    });
}

bool Editor::update(World& world, gfx::Context& ctx, gfx::Renderer& renderer) {
    show_main_menu_bar(world, ctx, renderer);

    bool dirty = false;

    DockSpace main_dock_space{
        "main_editor_space##dock",
        ImGui::GetMainViewport(), // The main editor dockspace occupies the entire window.
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking // We want to be able to show a menu bar.
        // and this window cannot be docked anywhere else.
    };

    // Display each active viewport
    for (gfx::Viewport vp : renderer.get_active_viewports()) {
        // Display the viewport
        SceneViewport viewport{vp, ctx, renderer, world};
    }

    console.display();
    dirty |= inspector.display(world);
    performance.display(ctx);
    return dirty;
}

void Editor::show_main_menu_bar(World& world, gfx::Context& ctx, gfx::Renderer& renderer) {
    MenuBar menu(true);

    {
        auto file = menu.submenu("File");
    }
    {
        auto window = menu.submenu("View");
        if (window.is_visible()) {
            window.item("New viewport", menu_values.create_viewport);
            ImGui::Separator();
            window.item("Inspector", inspector.is_visible());
            window.item("Performance Display", performance.is_visible());
        }
    }

    if (menu_values.create_viewport) {
        if (renderer.get_active_viewports().size() == gfx::MAX_VIEWPORTS) {
            LOG_WRITE(LogLevel::Warning, "Could not create new viewport, maximum amount reached.");
        } else {
            renderer.create_viewport(300, 300);
        }
        menu_values.create_viewport = false;
    }
}


} // namespace andromeda::editor