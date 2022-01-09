#include <andromeda/assets/assets.hpp>

#include <andromeda/graphics/context.hpp>
#include <andromeda/ecs/entity.hpp>
#include <andromeda/assets/loaders.hpp>

namespace andromeda {
namespace assets {

namespace impl {

gfx::Context* gfx_context = nullptr;
World* world = nullptr;

template<>
Handle<gfx::Texture> load_priv<gfx::Texture>(std::string const& path) {
    return gfx_context->request_texture(path);
}

template<>
Handle<gfx::Mesh> load_priv<gfx::Mesh>(std::string const& path) {
    return gfx_context->request_mesh(path);
}

template<>
Handle<gfx::Material> load_priv<gfx::Material>(std::string const& path) {
    return gfx_context->request_material(path);
}

template<>
Handle<ecs::entity_t> load_priv<ecs::entity_t>(std::string const& path) {
    LOG_FORMAT(LogLevel::Info, "Loading entity at path {}", path);
    Handle<ecs::entity_t> handle = assets::impl::insert_pending<ecs::entity_t>();
    ::andromeda::impl::load_entity(*gfx_context, *world, handle, path);
    assets::impl::set_path(handle, path);
    return handle;
}

template<>
Handle<gfx::Environment> load_priv<gfx::Environment>(std::string const& path) {
    return gfx_context->request_environment(path);
}

}

template<>
void unload<gfx::Texture>(Handle<gfx::Texture> handle) {
    impl::gfx_context->free_texture(handle);
}

template<>
void unload<gfx::Mesh>(Handle<gfx::Mesh> handle) {
    impl::gfx_context->free_mesh(handle);
}

template<>
void unload<gfx::Material>(Handle<gfx::Material> handle) {
    // Do nothing, materials don't actually own any resources.
}

template<>
void unload<ecs::entity_t>(Handle<ecs::entity_t> handle) {
    // Not implemented
    // TODO: Deleting entities (both from blueprints and main ECS)
}

template<>
void unload<gfx::Environment>(Handle<gfx::Environment> handle) {
    impl::gfx_context->free_environment(handle);
}

} // namespace assets
} // namespace andromeda