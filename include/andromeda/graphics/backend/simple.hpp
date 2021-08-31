#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>

namespace andromeda::gfx::backend {

/**
 * @class SimpleRenderer 
 * @brief Very simple renderer implementation, mostly temporary to get things set up.
*/
class SimpleRenderer : public RendererBackend {
public:
	/**
	 * @brief Create a SimpleRenderer
	 * @param ctx Reference to the graphics context.
	*/
	SimpleRenderer(gfx::Context& ctx);

	/**
	 * @brief Renders a scene to a viewport.
	 * @param graph Render graph to add the renderpasses to.
	 * @param ifc In-flight context for allocating scratch buffers.
	 * @param viewport Viewport to render to.
	 * @param scene SceneDescription with all necessary information.
	*/
	void render_scene(ph::RenderGraph& graph, ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) override;

	/**
	 * @brief Get a list of debug views associated with a viewport.
	 * @param viewport The viewport to get debug views for.
	 * @return A vector of strings, each element being the name of a debug attachment that can be displayed.
	 *		   SimpleRenderer only exposes a depth attachment.
	*/
	std::vector<std::string> debug_views(gfx::Viewport viewport) override;

	/**
	 * @brief Called by the renderer when a viewport is resized.
	 * @param viewport The viewport that is being resized.
	 * @param width The new width of the viewport.
	 * @param height The new height of the viewport
	*/
	void resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) override;

private:
	// Stores the names of every depth attachment used, indexed by viewport.
	std::vector<std::string> depth_attachments{};
};

} // namespace andromeda::gfx::backend