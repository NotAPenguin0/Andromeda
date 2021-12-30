#pragma once

#include <andromeda/graphics/forward.hpp>
#include <andromeda/util/handle.hpp>
#include <andromeda/world.hpp>


#include <string_view>

namespace andromeda {

namespace impl {

void load_texture(gfx::Context& ctx, Handle<gfx::Texture> handle, std::string_view path, uint32_t thread);
void load_mesh(gfx::Context& ctx, Handle<gfx::Mesh> handle, std::string_view path, uint32_t thread);
void load_material(gfx::Context& ctx, Handle<gfx::Material> handle, std::string_view path, uint32_t thread);
void load_entity(gfx::Context& ctx, World& world, Handle<ecs::entity_t> handle, std::string_view path); // thread parameter not needed
void load_environment(gfx::Context& ctx, Handle<gfx::Environment> handle, std::string_view path, uint32_t thread);

} // namespace impl
} // namespace andromeda