#include <andromeda/wsi/window.hpp>

#include <phobos/core/window_context.hpp>

namespace andromeda::wsi {

Window::Window(uint32_t width, uint32_t height, std::string_view title) {
	window = ph::create_window_context(title, width, height);
}

Window::~Window() {
	close();
}

void Window::close() {
	if (window) {
		ph::destroy_window_context(window);
		window = nullptr;
	}
}

void Window::poll_events() {
	window->poll_events();
}

uint32_t Window::width() const {
	return window->width;
}

uint32_t Window::height() const {
	return window->height;
}

bool Window::is_open() const {
	return window->is_open();
}

ph::WindowContext* Window::handle() {
	return window;
}

}