#ifndef ANDROMEDA_APPLICATION_HPP_
#define ANDROMEDA_APPLICATION_HPP_

#include <andromeda/core/context.hpp>
#include <andromeda/renderer/renderer.hpp>
#include <andromeda/world/world.hpp>
#include <andromeda/wsi/window.hpp>


namespace andromeda::wsi {

// Core class of the Andromeda Editor. This ties together the window, renderer, and other systems. It also takes care of the event loop
class Application {
public:
	Application(size_t width, size_t height, std::string_view title);
	~Application();

	void run();

private:
	std::unique_ptr<renderer::Renderer> renderer;
	wsi::Window window;
	Context context;
};

}

#endif