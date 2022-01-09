#pragma once

#include <phobos/context.hpp>

#include <andromeda/app/log.hpp>
#include <andromeda/app/wsi.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/graphics/mesh.hpp>
#include <andromeda/graphics/texture.hpp>
#include <andromeda/graphics/material.hpp>
#include <andromeda/graphics/environment.hpp>
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
     *	      not copy or movable.
     * @param window Reference to the windowing interface
     * @param logger Reference to the logger interface
     * @return A unique_ptr to the created graphics context.
    */
    static std::unique_ptr<Context> init(Window& window, Log& logger, thread::TaskScheduler& scheduler);

    /**
     * @brief Returns the delta time associated with the held window. This must be externally updated.
     * @return Time between this frame and the previous frame, in seconds.
     */
    float delta_time() const;

    inline thread::TaskScheduler& get_scheduler() { return scheduler; }

private:
    Context(ph::AppSettings settings, Window& window, thread::TaskScheduler& scheduler);

    thread::TaskScheduler& scheduler;
    Window& window;

    // load_priv() needs access to the request_XXX() functions.
    template<typename T>
    friend Handle<T> assets::impl::load_priv(std::string const&);

    // So does unload()
    template<typename T>
    friend void assets::unload(Handle<T>);

    /**
     * @brief Request a texture to be loaded. This will be done asynchronously.
     * @param path Path to the texture file.
     * @return A handle referring to the texture in the asset system.
    */
    Handle<gfx::Texture> request_texture(std::string const& path);

    /**
     * @brief Request a mesh to be loaded. This will be done asynchronously.
     * @param path Path to the mesh file
     * @return A handle referring to the mesh in the asset system
    */
    Handle<gfx::Mesh> request_mesh(std::string const& path);

    /**
     * @brief Request a material to be loaded. This will be done asynchronously.
     * @param path Path to the material file.
     * @return A handle referring to the material in the asset system.
    */
    Handle<gfx::Material> request_material(std::string const& path);

    /**
     * @brief Request an environment to be loaded asynchronously.
     * @param path Path to the environment (.env) file.
     * @return A handle referring to the environment in the asset system
     */
    Handle<gfx::Environment> request_environment(std::string const& path);

    /**
     * @brief Frees a texture. This will be done asynchronously.
     * @param handle A handle referring to the texture to free
    */
    void free_texture(Handle<gfx::Texture> handle);

    /**
     * @brief Frees a mesh. This will be done asynchronously.
     * @param handle A handle referring to the mesh to free.
    */
    void free_mesh(Handle<gfx::Mesh> handle);

    /**
     * @brief Free an environment asynchronously.
     * @param handle A handle referring to the environment to free.
     */
    void free_environment(Handle<gfx::Environment> handle);
};

} // namespace gfx
} // namespace andromeda