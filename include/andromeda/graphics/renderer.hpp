#pragma once

#include <andromeda/app/wsi.hpp>
#include <andromeda/graphics/backend/renderer_backend.hpp>
#include <andromeda/graphics/backend/debug_geometry.hpp>
#include <andromeda/graphics/context.hpp>
#include <andromeda/graphics/scene_description.hpp>
#include <andromeda/graphics/viewport.hpp>
#include <andromeda/world.hpp>

#include <array>
#include <optional>

namespace andromeda {
namespace gfx {

/**
 * @class Renderer 
 * @brief Acts as an interface into the rendering system. Responsible for rendering the world.
*/
class Renderer {
public:
    /**
     * @brief Initializes the renderer.
     * @param ctx Reference to the graphics context.
     * @param window Reference to the application window.
    */
    Renderer(gfx::Context& ctx, Window& window);

    /**
     * @brief Deinitializes the renderer. Must be called on shutdown while the context is still valid.
     * @param ctx Reference to the graphics context.
    */
    void shutdown(gfx::Context& ctx);

    /**
     * @brief Render a single frame. Must be called on the main thread.
     * @param ctx Reference to the graphics context.
     * @param world Reference to the world to render.
     * @param dirty Whether the scene was modified since last frame
    */
    void render_frame(gfx::Context& ctx, World const& world, bool dirty);

    /**
     * @brief Creates a new viewport with given size.
     * @param width Width of the viewport in pixels.
     * @param height Height of the viewport in pixels.
     * @param camera Optionally a camera entity. As long as this is empty nothing will be rendered to
     *				 the viewport.
     * @return A gfx::Viewport if there are still viewport slots open, std::nullopt otherwise.
    */
    std::optional<gfx::Viewport> create_viewport(uint32_t width, uint32_t height, ecs::entity_t camera = ecs::no_entity);

    /**
     * @brief Resizes a viewport. This won't do an actual resize if the size already matches.
     * @param ctx Reference to the graphics context.
     * @param viewport Viewport to resize.
     * @param width New width of the viewport.
     * @param height New height of the viewport.
    */
    void resize_viewport(gfx::Context& ctx, gfx::Viewport& viewport, uint32_t width, uint32_t height);

    /**
     * @brief Get all currently active viewports.
     * @return A vector containing every active viewport.
    */
    std::vector<gfx::Viewport> get_active_viewports();

    /**
     * @brief Sets the camera associated with a viewport.
     * @param viewport The viewport to set the camera for.
     * @param camera The camera to be used for this viewport.
    */
    void set_viewport_camera(gfx::Viewport& viewport, ecs::entity_t camera);

    /**
     * @brief Destroys a viewport.
     * @param viewport The viewport to destroy
    */
    void destroy_viewport(gfx::Viewport viewport);

    /**
     * @brief Get a list of debug views associated with a viewport.
     * @param viewport The viewport to get debug views for.
     * @return A vector of strings, each element being the name of a debug attachment that can be displayed.
    */
    std::vector<std::string> get_debug_views(gfx::Viewport viewport);

private:
    std::unique_ptr<backend::RendererBackend> impl{};
    SceneDescription scene;
    backend::DebugGeometryList debug_geometry;

    struct ViewportData {
        // The actual viewport
        gfx::Viewport vp{};
        // Whether it's currently in use or not.
        bool in_use = false;
    };
    std::array<ViewportData, gfx::MAX_VIEWPORTS> viewports{};

    void fill_scene_description(World const& world);
};

} // namespace gfx
} // namespace andromeda