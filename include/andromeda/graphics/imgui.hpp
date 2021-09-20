#pragma once

#include <andromeda/app/wsi.hpp>
#include <andromeda/graphics/context.hpp>

#include <phobos/render_graph.hpp>
#include <imgui/imgui.h>

#include <IconsFontAwesome5.h>

namespace andromeda::gfx::imgui {

/**
 * @brief Initializes the imgui rendering backend. Must be called on the main thread.
 * @param ctx Reference to the graphics context.
*/
void init(gfx::Context& ctx, Window& window);

/**
 * @brief Call at the beginning of every frame to ensure ImGui is properly updated.
*/
void new_frame();

/**
 * @brief Renders a single ImGui frame. 
 * @param graph Reference to the render graph to populate.
 * @param ifc Reference to the current frame's in-flight context.
 * @param attachment Name of the render target to render the UI to.
*/
void render(ph::RenderGraph& graph, ph::InFlightContext& ifc, std::string_view target);

/**
 * @brief Cleans up resources used by the imgui rendering backend.
*/
void shutdown();

/**
 * @brief Get the texture ID for an image.
 * @param view Image view to get the texture ID of.
 * @return ImTextureID pointing to the given ImageView.
*/
ImTextureID get_texture_id(ph::ImageView view);

/**
 * @brief Reloads the font data. Must be called after adding new fonts for them to work.
 *		  Can only be called if no ImGui elements are currently being rendered.
*/
void reload_fonts();

} // namespace andromeda::gfx::imgui