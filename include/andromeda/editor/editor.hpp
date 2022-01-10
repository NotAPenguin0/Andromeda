#pragma once

#include <andromeda/app/wsi.hpp>
#include <andromeda/graphics/imgui.hpp>
#include <andromeda/graphics/renderer.hpp>
#include <andromeda/world.hpp>

#include <andromeda/editor/console.hpp>
#include <andromeda/editor/inspector.hpp>
#include <andromeda/editor/performance.hpp>

namespace andromeda {
namespace editor {

/**
 * @class Editor 
 * @brief Manages the editor UI and update loop.
*/
class Editor {
public:
    /**
     * @brief Initializes the editor. ImGui must be initialized (but the renderer does not have to be).
     *		  This function will load fonts, so you must call gfx::imgui::reload_fonts() if the renderer
     *		  was already initialized before calling this.
     * @param ctx Reference to the graphics context.
     * @param window Reference to the application window.
    */
    Editor(gfx::Context& ctx, Window& window);

    /**
     * @brief Must be called every frame to update the UI.
     * @param world Reference to the world to build the editor for.
     * @param ctx Reference to the graphics context
     * @param renderer Reference to the rendering interface.
    */
    void update(World& world, gfx::Context& ctx, gfx::Renderer& renderer);

private:
    ImFont* font = nullptr;
    Console console;
    Inspector inspector;
    PerformanceDisplay performance;

    void show_main_menu_bar(World& world, gfx::Context& ctx, gfx::Renderer& renderer);

    struct MenuValues {
        bool create_viewport = false;
    } menu_values;
};

} // namespace editor
} // namespace andromeda