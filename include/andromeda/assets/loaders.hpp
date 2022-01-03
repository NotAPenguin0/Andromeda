#pragma once

#include <andromeda/graphics/forward.hpp>
#include <andromeda/util/handle.hpp>
#include <andromeda/world.hpp>


#include <string_view>

namespace andromeda {

namespace impl {

void load_texture(gfx::Context& ctx, Handle<gfx::Texture> handle, std::string_view path, uint32_t thread);
// Load a 1x1 single color texture in sRGBA8 format
void load_1x1_texture(gfx::Context& ctx, Handle<gfx::Texture> handle, uint8_t bytes[4], uint32_t thread);
void load_mesh(gfx::Context& ctx, Handle<gfx::Mesh> handle, std::string_view path, uint32_t thread);
void load_material(gfx::Context& ctx, Handle<gfx::Material> handle, std::string_view path, uint32_t thread);
void load_entity(gfx::Context& ctx, World& world, Handle<ecs::entity_t> handle, std::string_view path); // thread parameter not needed
void load_environment(gfx::Context& ctx, Handle<gfx::Environment> handle, std::string_view path, uint32_t thread);

} // namespace impl
} // namespace andromeda