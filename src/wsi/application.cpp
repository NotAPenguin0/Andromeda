#include <andromeda/wsi/application.hpp>

namespace andromeda::wsi {

Application::Application(size_t width, size_t height, std::string_view title)
	: window(width, height, title), renderer(window) {

}

void Application::run() {
	while (window.is_open()) {
		window.poll_events();
	}
}

}