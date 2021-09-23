#include <andromeda/assets/assets.hpp>

#include <andromeda/graphics/context.hpp>

namespace andromeda {
namespace assets {

namespace impl {

template<>
Handle<gfx::Texture> load_priv<gfx::Texture>(gfx::Context& ctx, std::string const& path) {
	return ctx.request_texture(path);
}

template<>
Handle<gfx::Mesh> load_priv<gfx::Mesh>(gfx::Context& ctx, std::string const& path) {
	return ctx.request_mesh(path);
}

}

template<>
void unload<gfx::Texture>(gfx::Context& ctx, Handle<gfx::Texture> handle) {
	ctx.free_texture(handle);
}

template<>
void unload<gfx::Mesh>(gfx::Context& ctx, Handle<gfx::Mesh> handle) {
	ctx.free_mesh(handle);
}

} // namespace assets
} // namespace andromeda