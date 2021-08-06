#pragma once

#include <phobos/context.hpp>

#include <andromeda/app/log.hpp>
#include <andromeda/app/wsi.hpp>

#include <memory>

namespace andromeda {
namespace gfx {

/**
 * @class Context 
 * @brief Contains the core graphics context. Any phobos functionality is inherited by this class.
*/
class Context : public ph::Context {
public:
	/**
	 * @brief Creates a context and returns it through a unique_ptr, since the context by itself is
	 *	      not copy or moveable.
	 * @param window Reference to the windowing interface
	 * @param logger Reference to the logger interface
	 * @return A unique_ptr to the created graphics context.
	*/
	static std::unique_ptr<Context> init(Window& window, Log& logger);
private:
	Context(ph::AppSettings settings);
};

} // namespace gfx
} // namespace andromeda