#pragma once

#include <andromeda/graphics/imgui.hpp>
#include <andromeda/graphics/renderer.hpp>
#include <andromeda/graphics/viewport.hpp>

namespace andromeda::editor {

/**
 * @class SceneViewport 
 * @brief Class used to add a window to the UI that can display and interact with a scene viewport.
*/
class SceneViewport {
public:
	/**
	 * @brief Creates a window to display a viewport of a scene.
	 * @param viewport The viewport to display.
	 * @param ctx Reference to the graphics context.
	 * @param renderer Reference to the rendering interface.
	 * @param world The world the viewport is displaying.
	*/
	SceneViewport(gfx::Viewport viewport, gfx::Context& ctx, gfx::Renderer& renderer, World const& world);

	/**
	 * @brief Returns whether the window is visible. This is effectively the result of
	 *		  ImGui::Begin().
	 * @return true if the window is visible and additional content may be added.
	*/
	bool is_visible() const;

private:
	bool visible = false;

	// Persistent data per viewport
	struct PerViewportStatic {
		PerViewportStatic() : is_open(true) {}

		std::unordered_map<std::string, bool> debug_views;
		bool is_open = true;
	};

	static inline std::array<PerViewportStatic, gfx::MAX_VIEWPORTS> per_viewport{};

	void show_viewport(gfx::Viewport viewport, gfx::Context& ctx, gfx::Renderer& renderer, World const& world);
	void show_menu_bar(gfx::Viewport viewport, gfx::Renderer& renderer);
	void show_panel(gfx::Viewport viewport, gfx::Renderer& renderer, World const& world);
	void show_debug_views(gfx::Viewport viewport, gfx::Context& ctx, gfx::Renderer& renderer);
};

} // namespace andromeda::editor