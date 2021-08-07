#pragma once

#include <andromeda/util/handle.hpp>

#include <string_view>

namespace andromeda {

namespace gfx {
	class Context;

	struct Texture;
} // namespace gfx

namespace impl {

void load_texture(gfx::Context& ctx, Handle<gfx::Texture> handle, std::string_view path, uint32_t thread);

} // namespace impl
} // namespace andromeda