#include <andromeda/wsi/window.hpp>

int main() {
	andromeda::wsi::Window window(1280, 720, "Andromeda Editor");
	while (window.is_open()) {
		window.poll_events();
	}
	window.close();
}