#include <andromeda/wsi/application.hpp>

int main() {
	using namespace andromeda;

	wsi::Application app(1280, 710, "Andromeda Editor");
	app.run();
}