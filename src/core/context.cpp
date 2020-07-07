#include <andromeda/core/context.hpp>

#include <phobos/core/vulkan_context.hpp>

#include <andromeda/assets/texture.hpp>
#include <andromeda/assets/mesh.hpp>
#include <andromeda/assets/assets.hpp>
#include <andromeda/world/world.hpp>

#include <andromeda/util/types.hpp>

#include <andromeda/assets/importers/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace andromeda {

Context::~Context() = default;

static uint32_t format_byte_size(vk::Format format) {
	switch (format) {
	case vk::Format::eR8G8B8A8Unorm:
	case vk::Format::eR8G8B8A8Srgb:
		return 4;
	case vk::Format::eR8G8B8Unorm:
	case vk::Format::eR8G8B8Srgb:
		return 3;
	default:
		return 0;
	}
}

struct TextureLoadInfo {
	Handle<Texture> handle;
	std::string_view path;
	bool srgb;
	Context* ctx;
};

static void do_texture_load(ftl::TaskScheduler* scheduler, TextureLoadInfo load_info) {
	ph::VulkanContext& vulkan = *load_info.ctx->vulkan;

	int width, height, channels;
	// Always load image as rgba
	uint8_t* data = stbi_load(load_info.path.data(), &width, &height, &channels, 4);

	Texture texture;

	vk::Format format = load_info.srgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

	texture.image = ph::create_image(vulkan, width, height, ph::ImageType::Texture, format);
	// Upload data
	uint32_t size = width * height * format_byte_size(format);
	ph::RawBuffer staging = ph::create_buffer(vulkan, size, ph::BufferType::TransferBuffer);
	std::byte* staging_mem = ph::map_memory(vulkan, staging);
	std::memcpy(staging_mem, data, size);
	ph::unmap_memory(vulkan, staging);

	// We need to know the current thread index to allocate these command buffers from the correct thread
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();
	
	// Command buffer recording and execution will happen as follows
	// 1. Record commands for transfer [TRANSFER QUEUE]
	// 2. Submit commands to [TRANSFER QUEUE] with signalSemaphore = transfer_done
	// 3. Record layout transition to ShaderReadOnlyOptimal on [GRAPHICS QUEUE]
	// 4. Submit commands to [GRAPHICS QUEUE] with waitSemaphore = transfer_done and signalFence = transition_done
	// 5. Wait for fence transition_done 
	// 6. Done

	// 1. Record commands for transfer 
	vk::CommandBuffer cmd_buf = vulkan.transfer->begin_single_time(thread_index);
	ph::transition_image_layout(cmd_buf, texture.image, vk::ImageLayout::eTransferDstOptimal);
	ph::copy_buffer_to_image(cmd_buf, ph::whole_buffer_slice(vulkan, staging), texture.image);

	// Submit queue ownership transfer
	vulkan.transfer->release_ownership(cmd_buf, texture.image, *vulkan.graphics);

	// 2. Submit to transfer queue
	vk::Semaphore transfer_done = vulkan.device.createSemaphore({});
	vulkan.transfer->end_single_time(cmd_buf, nullptr, {}, nullptr, transfer_done);

	// 3. Record commands for layout transition
	// First acquire ownership of the image, then exeucte transition
	vk::CommandBuffer transition_cmd = vulkan.graphics->begin_single_time(thread_index);
	vulkan.graphics->acquire_ownership(transition_cmd, texture.image, *vulkan.transfer);
	ph::transition_image_layout(transition_cmd, texture.image, vk::ImageLayout::eShaderReadOnlyOptimal);

	// 4. Submit to graphics queue
	vk::Fence transition_done = vulkan.device.createFence({});
	vulkan.graphics->end_single_time(transition_cmd, transition_done, vk::PipelineStageFlagBits::eTransfer, transfer_done);

	// 5. Wait for fence
	vulkan.device.waitForFences(transition_done, true, std::numeric_limits<uint64_t>::max());

	// 6. Done, do some cleanup and exit
	vulkan.device.destroyFence(transition_done);
	vulkan.device.destroySemaphore(transfer_done);
	vulkan.transfer->free_single_time(cmd_buf, thread_index);

	// Don't forget to destroy the staging buffer
	ph::destroy_buffer(vulkan, staging);
	// Create image view
	texture.view = ph::create_image_view(vulkan.device, texture.image);

	// Push texture to the asset system and set state to Ready
	assets::finalize_load(load_info.handle, std::move(texture));

	io::log("Finished load {}", load_info.path);
	// Cleanup
	stbi_image_free(data);
}

struct MeshLoadInfo {
	Handle<Mesh> handle;
	std::string_view path;
	Context* ctx;
};

static void load_mesh_data(ftl::TaskScheduler* scheduler, MeshLoadInfo& load_info, aiMesh* mesh) {
	constexpr size_t vtx_size = 3 + 3 + 3 + 2;
	std::vector<float> verts(mesh->mNumVertices * vtx_size);
	for (size_t i = 0; i < mesh->mNumVertices; ++i) {
		size_t const index = i * vtx_size;
		// Position
		verts[index] = mesh->mVertices[i].x;
		verts[index + 1] = mesh->mVertices[i].y;
		verts[index + 2] = mesh->mVertices[i].z;
		// Normal
		verts[index + 3] = mesh->mNormals[i].x;
		verts[index + 4] = mesh->mNormals[i].y;
		verts[index + 5] = mesh->mNormals[i].z;
		// Tangent
		verts[index + 6] = mesh->mTangents[i].x;
		verts[index + 7] = mesh->mTangents[i].y;
		verts[index + 8] = mesh->mTangents[i].z;
		// TexCoord
		verts[index + 9] = mesh->mTextureCoords[0][i].x;
		verts[index + 10] = mesh->mTextureCoords[0][i].y;
	}

	std::vector<uint32_t> indices;
	indices.reserve(mesh->mNumFaces * 3);
	for (size_t i = 0; i < mesh->mNumFaces; ++i) {
		aiFace const& face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}
	
	Mesh result;
	ph::VulkanContext& vulkan = *load_info.ctx->vulkan;
	result.vertices = ph::create_buffer(vulkan, verts.size() * sizeof(float), ph::BufferType::VertexBuffer);
	result.indices = ph::create_buffer(vulkan, indices.size() * sizeof(uint32_t), ph::BufferType::IndexBuffer);
	result.indices_size = indices.size();

	auto fill_staging = [&vulkan](void* data, uint32_t size) -> ph::RawBuffer {
		ph::RawBuffer staging = ph::create_buffer(vulkan, size, ph::BufferType::TransferBuffer);
		std::byte* memory = ph::map_memory(vulkan, staging);
		std::memcpy(memory, data, size);
		ph::unmap_memory(vulkan, staging);
		return staging;
	};

	ph::RawBuffer vertex_staging = fill_staging(verts.data(), verts.size() * sizeof(float));
	ph::RawBuffer index_staging = fill_staging(indices.data(), indices.size() * sizeof(uint32_t));

	// Issue buffer copy commands to transfer queue
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();
	vk::CommandBuffer cmd_buf = vulkan.transfer->begin_single_time(thread_index);
	
	ph::copy_buffer(vulkan, cmd_buf, vertex_staging, result.vertices, verts.size() * sizeof(float));
	ph::copy_buffer(vulkan, cmd_buf, index_staging, result.indices, indices.size() * sizeof(uint32_t));

	vulkan.transfer->release_ownership(cmd_buf, result.vertices, *vulkan.graphics);
	vulkan.transfer->release_ownership(cmd_buf, result.indices, *vulkan.graphics);

	vk::Semaphore copy_done = vulkan.device.createSemaphore({});
	vulkan.transfer->end_single_time(cmd_buf, nullptr, {}, nullptr, copy_done);

	vk::Fence ownership_done = vulkan.device.createFence({});
	vk::CommandBuffer ownership_cbuf = vulkan.graphics->begin_single_time(thread_index);
	vulkan.graphics->acquire_ownership(ownership_cbuf, result.vertices, *vulkan.transfer);
	vulkan.graphics->acquire_ownership(ownership_cbuf, result.indices, *vulkan.transfer);
	vulkan.graphics->end_single_time(ownership_cbuf, ownership_done, vk::PipelineStageFlagBits::eTransfer, copy_done);

	vulkan.device.waitForFences(ownership_done, true, std::numeric_limits<uint64_t>::max());
	// Cleanup, transfer operation complete
	vulkan.device.destroyFence(ownership_done);
	vulkan.device.destroySemaphore(copy_done);

	vulkan.transfer->free_single_time(cmd_buf, thread_index);
	vulkan.graphics->free_single_time(ownership_cbuf, thread_index);

	ph::destroy_buffer(vulkan, vertex_staging);
	ph::destroy_buffer(vulkan, index_staging);

	assets::finalize_load(load_info.handle, std::move(result));
}

static void do_mesh_load(ftl::TaskScheduler* scheduler, MeshLoadInfo load_info) {
	Assimp::Importer importer;
	constexpr int postprocess = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace;
	aiScene const* scene = importer.ReadFile(std::string(load_info.path), postprocess);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
		!scene->mRootNode) {
		throw std::runtime_error("Failed to load model");
	}

	for (size_t i = 0; i < scene->mNumMeshes; ++i) {
		// Note that the assimp::Importer destructor takes care of freeing the aiScene
		load_mesh_data(scheduler, load_info, scene->mMeshes[i]);
		break;
	}
}

Handle<Texture> Context::request_texture(std::string_view path, bool srgb) {
	Handle<Texture> handle = assets::insert_pending<Texture>();
	tasks->launch(do_texture_load, TextureLoadInfo{ handle, path, srgb, this });
	return handle;
}

Handle<Mesh> Context::request_mesh(std::string_view path) {
	Handle<Mesh> handle = assets::insert_pending<Mesh>();
	tasks->launch(do_mesh_load, MeshLoadInfo{ handle, path, this });
	return handle;
}

Handle<Mesh> Context::request_mesh(float const* vertices, uint32_t size, uint32_t const* indices, uint32_t index_count) {
	Mesh mesh;

	uint32_t const byte_size = size * sizeof(float);
	mesh.vertices = ph::create_buffer(*vulkan, byte_size, ph::BufferType::VertexBuffer);
	ph::RawBuffer staging = ph::create_buffer(*vulkan, byte_size, ph::BufferType::TransferBuffer);
	std::byte* staging_mem = ph::map_memory(*vulkan, staging);
	std::memcpy(staging_mem, vertices, byte_size);
	ph::unmap_memory(*vulkan, staging);

	uint32_t index_byte_size = index_count * sizeof(uint32_t);
	mesh.indices = ph::create_buffer(*vulkan, index_byte_size, ph::BufferType::IndexBuffer);
	mesh.indices_size = index_count;
	ph::RawBuffer index_staging = ph::create_buffer(*vulkan, index_byte_size, ph::BufferType::TransferBuffer);
	staging_mem = ph::map_memory(*vulkan, index_staging);
	std::memcpy(staging_mem, indices, index_byte_size);
	ph::unmap_memory(*vulkan, index_staging);

	vk::CommandBuffer cmd_buf = vulkan->graphics->begin_single_time(0);
	ph::copy_buffer(*vulkan, cmd_buf, staging, mesh.vertices, byte_size);
	ph::copy_buffer(*vulkan, cmd_buf, index_staging, mesh.indices, index_byte_size);
	vulkan->graphics->end_single_time(cmd_buf);
	vulkan->graphics->wait_idle();
	vulkan->graphics->free_single_time(cmd_buf, 0);

	return assets::take(std::move(mesh));
}


Handle<EnvMap> Context::request_env_map(std::string_view path) {
	Handle<EnvMap> handle = assets::insert_pending<EnvMap>();
	EnvMapLoadInfo load_info{ handle, this, std::string(path) };
//	envmap_loader->load(&tasks->get_scheduler(), load_info);
	tasks->launch([this](ftl::TaskScheduler* scheduler, EnvMapLoadInfo load_info) -> void {
		envmap_loader->load(scheduler, std::move(load_info));
		}, std::move(load_info));
	return handle;
}

}