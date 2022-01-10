#pragma once

#include <andromeda/graphics/backend/renderer_backend.hpp>

namespace andromeda::gfx { class Renderer; }

namespace andromeda::gfx::backend {

class DebugGeometryList {
public:
    /**
     * @brief Adds a line to the debug draw list.
     * @param viewport Current rendering viewport.
     * @param p1 First point of the line, in world space.
     * @param p2 Second point of the line, in world space.
     */
    void add_line(gfx::Viewport const& viewport, glm::vec3 p1, glm::vec3 p2);

    /**
     * @brief Sets the color for all subsequent debug draws added to the list.
     * @param viewport Current rendering viewport.
     * @param color New color to use.
     */
    void set_color(gfx::Viewport const& viewport, glm::vec3 color);

private:
    friend class andromeda::gfx::Renderer;

    enum class CommandType {
        DrawLine,
        DrawTriangle,
        PushColor
    };

    struct DrawCommand {
        // Offset in the VBO of this draw command's vertices.
        uint32_t vbo_offset = 0;
        uint32_t num_vertices = 0;
    };

    struct PushColorCommand {
        glm::vec3 color;
    };

    struct Command {
        CommandType type{};
        union {
            DrawCommand draw{};
            PushColorCommand color;
        };
    };

    struct PerViewport {
        std::vector<Command> commands;
        std::vector<glm::vec3> vertices;
    } vp[gfx::MAX_VIEWPORTS];

    /**
     * @brief Initialize the debug drawer. This will create pipelines for the debug draw system.
     * @param ctx Reference to the graphics context.
     */
    static void initialize(gfx::Context& ctx);

    /**
     * @brief Clear draw list for a viewport.
     * @param vp Viewport to clear draw list for
     */
    void clear(gfx::Viewport const& vp);

    /**
     * @brief Builds a renderpass to draw all debug geometry.
     * @param vp Current rendering viewport.
     * @param ctx Reference to the graphics context.
     * @param ifc Reference to the in flight context.
     * @param pv Projection-view matrix for this viewport.
     * @return ph::Pass to be inserted into the render graph.
     */
    ph::Pass build_render_pass(gfx::Viewport const& vp, gfx::Context& ctx, ph::InFlightContext& ifc, glm::mat4 const& pv);
};

namespace impl {
extern DebugGeometryList* _debug_geometry_list_ptr;
}

#define DEBUG_DRAW_LINE(vp, p1, p2) ::andromeda::gfx::backend::impl::_debug_geometry_list_ptr->add_line(vp, p1, p2)
#define DEBUG_SET_COLOR(vp, color) ::andromeda::gfx::backend::impl::_debug_geometry_list_ptr->set_color(vp, color)

}