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
Handle<Texture> load(Context& ctx, std::string_view path) {
	auto start = ch::system_clock::now();
	
	int width, height, channels;
	// Always load image as rgba
	importers::byte* data = stbi_load(path.data(), &width, &height, &channels, 4);

	auto end = ch::system_clock::now();
	io::log("PNG with size {}x{} loaded in {} ms", width, height, 
		ch::duration_cast<ch::milliseconds>(end - start).count());

	auto handle = ctx.request_texture(width, height, vk::Format::eR8G8B8A8Srgb, data);
	stbi_image_free(data);
	return handle;
}

static Handle<Mesh> load_mesh_data(Context& ctx, aiMesh* mesh) {
    constexpr size_t vtx_size = 5;
    std::vector<float> verts(mesh->mNumVertices * vtx_size);
    for (size_t i = 0; i < mesh->mNumVertices; ++i) {
        size_t const index = i * vtx_size;
        // Position
        verts[index] = mesh->mVertices[i].x;
        verts[index + 1] = mesh->mVertices[i].y;
        verts[index + 2] = mesh->mVertices[i].z;

        verts[index + 3] = mesh->mTextureCoords[0][i].x;
        verts[index + 4] = mesh->mTextureCoords[0][i].y;
    }
    
    std::vector<uint32_t> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (size_t i = 0; i < mesh->mNumFaces; ++i) {
        aiFace const& face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return ctx.request_mesh(verts.data(), verts.size(), indices.data(), indices.size());
}

template<>
Handle<Mesh> load(Context& ctx, std::string_view path) {
    Assimp::Importer importer;
    constexpr int postprocess = aiProcess_Triangulate | aiProcess_FlipUVs;
    aiScene const* scene = importer.ReadFile(std::string(path), postprocess);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        throw std::runtime_error("Failed to load model");
    }

    for (size_t i = 0; i < scene->mNumMeshes; ++i) {
        // Note that the assimp::Importer destructor takes care of freeing the aiScene
        return load_mesh_data(ctx, scene->mMeshes[i]);
    }

    throw std::runtime_error("model had no meshes to load!");
}

}
}