#include <andromeda/app/application.hpp>

int main(int argc, char** argv) {
	andromeda::Application app{ argc, argv };
	return app.run();
}