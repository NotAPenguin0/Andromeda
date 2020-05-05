#ifndef ANDROMEDA_RENDERER_HPP_
#define ANDROMEDA_RENDERER_HPP_

#include <andromeda/wsi/window.hpp>
#include <andromeda/core/context.hpp>
#include <andromeda/renderer/render_database.hpp>
#include <andromeda/renderer/geometry_pass.hpp>

#include <phobos/renderer/render_attachment.hpp>
#include <phobos/pipeline/pipeline.hpp>

namespace andromeda::renderer {

class Renderer {
public:
	Renderer(Context& ctx);
	~Renderer();

	// Renders the world (and UI) to the screen. Before calling this, you are not allowed to directly modify in-use GPU resources.
	void render(Context& ctx);

private:
	std::unique_ptr<ph::Renderer> vk_renderer;
	std::unique_ptr<ph::PresentManager> vk_present;

	RenderDatabase database;

	std::unique_ptr<GeometryPass> geometry_pass;
};

} // namespace andromeda::renderer

#endif