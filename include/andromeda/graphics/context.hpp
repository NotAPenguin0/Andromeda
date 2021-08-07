#pragma once

#include <phobos/context.hpp>

#include <andromeda/app/log.hpp>
#include <andromeda/app/wsi.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/graphics/texture.hpp>
#include <andromeda/thread/scheduler.hpp>
#include <andromeda/util/handle.hpp>

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
	static std::unique_ptr<Context> init(Window& window, Log& logger, thread::TaskScheduler& scheduler);

private:
	Context(ph::AppSettings settings, thread::TaskScheduler& scheduler);

	thread::TaskScheduler& scheduler;

	// load() needs access to the request_XXX() functions.
	template<typename T>
	friend Handle<T> assets::load(gfx::Context&, std::string_view);

	/**
	 * @brief Request a texture to be loaded. This will be done asynchronously.
	 * @param path Path to the texture file.
	 * @return A handle referring to the texture in the asset system.
	*/
	Handle<gfx::Texture> request_texture(std::string_view path);
};

} // namespace gfx
} // namespace andromeda