#ifndef ANDROMEDA_RENDERER_HPP_
#define ANDROMEDA_RENDERER_HPP_

#include <andromeda/wsi/window.hpp>

#include <andromeda/core/context.hpp>


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
};

} // namespace andromeda::renderer

#endif