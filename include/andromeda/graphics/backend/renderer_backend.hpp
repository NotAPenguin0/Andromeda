#pragma once

#include <andromeda/graphics/context.hpp>
#include <andromeda/graphics/scene_description.hpp>
#include <andromeda/graphics/viewport.hpp>

#include <phobos/render_graph.hpp>

#include <vector>
#include <string>

namespace andromeda::gfx::backend {

/**
 * @class RendererBackend 
 * @brief Interface for a rendering backend.
 *		  This class is used as a base class for different rendering implementations.
*/
class RendererBackend {
public:
	/**
	 * @brief Initialize the backend.
	 * @param ctx Reference to the graphics context.
	*/
	explicit inline RendererBackend(gfx::Context& ctx) : ctx(ctx) {}

    virtual ~RendererBackend() = default;

	/**
	 * @brief Renders a scene to a viewport.
	 * @param graph Render graph to add the renderpasses to.
	 * @param ifc In-flight context for allocating scratch buffers.
	 * @param viewport Viewport to render to.
	 * @param scene SceneDescription with all necessary information.
	*/
	virtual void render_scene(ph::RenderGraph& graph, ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) = 0;

	/**
	 * @brief Get a list of debug views associated with a viewport.
	 * @param viewport The viewport to get debug views for.
	 * @return A vector of strings, each element being the name of a debug attachment that can be displayed.
	 *		   The default implementation returns an empty vector.
	*/
	virtual inline std::vector<std::string> debug_views(gfx::Viewport viewport) {
		return {};
	}

	/**
	 * @brief Called by the renderer when a viewport is resized.
	 *		  Use this to resize attachments
	 * @param viewport The viewport that is being resized.
	 * @param width The new width of the viewport.
	 * @param height The new height of the viewport
	*/
	virtual void resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) = 0;

protected:
	gfx::Context& ctx;
};

} // namespace andromeda::gfx::backend