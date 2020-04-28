#include <andromeda/wsi/application.hpp>

#include <andromeda/ecs/registry.hpp>

namespace andromeda::wsi {

Application::Application(size_t width, size_t height, std::string_view title)
	: window(width, height, title), renderer(window) {

}

void Application::run() {
	ecs::registry ecs;
	while (window.is_open()) {
		window.poll_events();
	}
}

}