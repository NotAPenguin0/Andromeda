#pragma once

#include <andromeda/app/wsi.hpp>
#include <andromeda/app/log.hpp>
#include <andromeda/graphics/context.hpp>
#include <andromeda/thread/scheduler.hpp>
#include <andromeda/world.hpp>

#include <memory>

namespace andromeda {

/**
 * @class Application
 * @brief Manages the Andromeda application. Create an instance of this class to launch the software.
*/
class Application {
public:

	/**
	 * @brief Create an Application instance.
	 * @param argc Amount of command-line arguments.
	 * @param argv Command-line arguments.
	*/
	Application(int argc, char** argv);

	/**
	 * @brief Runs the application. Blocks the calling thread until the app is closed.
	 * @return An exit code for the system.
	*/
	int run();

private:
	// Used for all application logging
	std::unique_ptr<Log> log;
	// Main application window
	std::unique_ptr<Window> window;
	// Core graphics context
	std::unique_ptr<gfx::Context> graphics;
	// The world with all entities in it
	std::unique_ptr<World> world;
	// The task scheduler
	std::unique_ptr<thread::TaskScheduler> scheduler;
};

} // namespace andromeda