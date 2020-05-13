#include <andromeda/core/context.hpp>

#include <phobos/core/vulkan_context.hpp>

#include <andromeda/assets/texture.hpp>
#include <andromeda/assets/mesh.hpp>
#include <andromeda/assets/assets.hpp>
#include <andromeda/world/world.hpp>

#include <andromeda/util/types.hpp>

#include <andromeda/assets/importers/stb_image.h>

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

static void do_texture_load(ftl::TaskScheduler* scheduler, void* arg) {
	TextureLoadInfo* load_info = reinterpret_cast<TextureLoadInfo*>(arg);
	ph::VulkanContext& vulkan = *load_info->ctx->vulkan;

	int width, height, channels;
	// Always load image as rgba
	uint8_t* data = stbi_load(load_info->path.data(), &width, &height, &channels, 4);

	Texture texture;

	vk::Format format = load_info->srgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm;

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

	// 2. Submit to transfer queue
	vk::Semaphore transfer_done = vulkan.device.createSemaphore({});
	vulkan.transfer->end_single_time(cmd_buf, nullptr, {}, nullptr, transfer_done);

	// 3. Record commands for layout transition
	vk::CommandBuffer transition_cmd = vulkan.graphics->begin_single_time(thread_index);
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
	assets::finalize_load(load_info->handle, std::move(texture));

	io::log("Finished load {}", load_info->path);
	// Cleanup
	stbi_image_free(data);
	delete load_info;
}

Handle<Texture> Context::request_texture(std::string_view path, bool srgb) {
	Handle<Texture> handle = assets::insert_pending<Texture>();
	tasks->launch(do_texture_load, new TextureLoadInfo{ handle, path, srgb, this });
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

	vk::CommandBuffer cmd_buf = vulkan->graphics->begin_single_time();
	ph::copy_buffer(*vulkan, staging, mesh.vertices, byte_size);
	ph::copy_buffer(*vulkan, index_staging, mesh.indices, index_byte_size);
	vulkan->graphics->end_single_time(cmd_buf);
	vulkan->device.waitIdle();

	return assets::take(std::move(mesh));
}

}