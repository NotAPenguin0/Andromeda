#include <andromeda/assets/assets.hpp>

#include <andromeda/graphics/context.hpp>

namespace andromeda {
namespace assets {

template<>
Handle<gfx::Texture> load<gfx::Texture>(gfx::Context& ctx, std::string_view path) {
	return ctx.request_texture(path);
}

} // namespace assets
} // namespace andromeda