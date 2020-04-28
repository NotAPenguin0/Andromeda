#include <andromeda/wsi/window.hpp>

#include <andromeda/renderer/renderer.hpp>

int main() {
	andromeda::wsi::Window window(1280, 720, "Andromeda Editor");
	andromeda::renderer::Renderer renderer(window);
	while (window.is_open()) {
		window.poll_events();
	}
}