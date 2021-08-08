#pragma once

#include <andromeda/app/wsi.hpp>
#include <andromeda/graphics/context.hpp>
#include <andromeda/world.hpp>

namespace andromeda {
namespace gfx {

/**
 * @class Renderer 
 * @brief Acts as an interface into the rendering system. Responsible for rendering the world.
*/
class Renderer {
public:
	/**
	 * @brief Initializes the renderer.
	 * @param ctx Reference to the graphics context.
	 * @param window Reference to the application window.
	*/
	Renderer(gfx::Context& ctx, Window& window);

	/**
	 * @brief Deinitializes the renderer. Must be called on shutdown while the context is still valid.
	 * @param ctx Reference to the graphics context.
	*/
	void shutdown(gfx::Context& ctx);

	/**
	 * @brief Render a single frame. Must be called on the main thread.
	 * @param ctx Reference to the graphics context.
	 * @param world Reference to the world to render.
	*/
	void render_frame(gfx::Context& ctx, World const& world);
};

} // namespace gfx
} // namespace andromeda