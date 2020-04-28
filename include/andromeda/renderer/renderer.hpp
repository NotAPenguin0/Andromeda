#ifndef ANDROMEDA_RENDERER_HPP_
#define ANDROMEDA_RENDERER_HPP_

#include <andromeda/wsi/window.hpp>

#include <phobos/forward.hpp>
#include <memory>

namespace andromeda::renderer {

class Renderer {
public:
	Renderer(wsi::Window& window);
	~Renderer();

private:
	ph::VulkanContext* vk_context;
	std::unique_ptr<ph::Renderer> vk_renderer;
	std::unique_ptr<ph::PresentManager> vk_present;
};

} // namespace andromeda::renderer

#endif