#include <andromeda/core/context.hpp>

#include <phobos/core/vulkan_context.hpp>
#include <phobos/util/cmdbuf_util.hpp>

#include <andromeda/assets/texture.hpp>
#include <andromeda/assets/assets.hpp>
#include <andromeda/world/world.hpp>


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

Handle<Texture> Context::request_texture(uint32_t width, uint32_t height, vk::Format format, void* data) {
	Texture texture;

	// Create texture
	texture.image = ph::create_image(*vulkan, width, height, ph::ImageType::Texture, format);
	// Upload data
	uint32_t size = width * height * format_byte_size(format);
	ph::RawBuffer staging = ph::create_buffer(*vulkan, size, ph::BufferType::TransferBuffer);
	std::byte* staging_mem = ph::map_memory(*vulkan, staging);
	std::memcpy(staging_mem, data, size);
	ph::unmap_memory(*vulkan, staging);
	// Copy the data to the image
	vk::CommandBuffer cmd_buf = ph::begin_single_time_command_buffer(*vulkan);
	ph::transition_image_layout(cmd_buf, texture.image, vk::ImageLayout::eTransferDstOptimal);
	ph::copy_buffer_to_image(cmd_buf, ph::whole_buffer_slice(*vulkan, staging), texture.image);
	ph::transition_image_layout(cmd_buf, texture.image, vk::ImageLayout::eShaderReadOnlyOptimal);
	ph::end_single_time_command_buffer(*vulkan, cmd_buf);
	// Don't forget to destroy the staging buffer
	ph::destroy_buffer(*vulkan, staging);
	// Create image view
	texture.view = ph::create_image_view(vulkan->device, texture.image);

	return assets::take<Texture>(std::move(texture));
}

}