#include <andromeda/assets/assets.hpp>

#include <andromeda/assets/importers/png.hpp>
#include <andromeda/assets/importers/stb_image.h>
#include <chrono>
namespace ch = std::chrono;

#include <vulkan/vulkan.hpp>

#include <andromeda/util/log.hpp>
#include <fmt/chrono.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace andromeda {

class Texture;
class Mesh;

namespace assets {

template<>
Handle<Texture> load(Context& ctx, std::string_view path, bool srgb) {
    return ctx.request_texture(path, srgb);
}

template<>
Handle<Mesh> load(Context& ctx, std::string_view path) {
    return ctx.request_mesh(path);
}

}
}