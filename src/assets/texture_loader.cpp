#include <andromeda/assets/loaders.hpp>

#include <andromeda/graphics/context.hpp>
#include <andromeda/graphics/texture.hpp>

#include <assetlib/asset_file.hpp>
#include <assetlib/texture.hpp>

namespace andromeda {
namespace impl {

static uint32_t format_byte_size(VkFormat format) {
	switch (format) {
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SRGB:
		return 4;
	case VK_FORMAT_R8G8B8_UNORM:
	case VK_FORMAT_R8G8B8_SRGB:
		return 3;
	default:
		return 0;
	}
}

static uint32_t get_full_image_byte_size(uint32_t width, uint32_t height, uint32_t mip_levels, VkFormat format) {
	// Total amount of pixels
	uint32_t pixels = 0;
	for (uint32_t level = 0; level < mip_levels; ++level) {
		pixels += (width / pow(2, level)) * (height / pow(2, level));
	}
	return pixels * format_byte_size(format);
}


void load_texture(gfx::Context& ctx, Handle<gfx::Texture> handle, std::string_view path, uint32_t thread) {
	assetlib::AssetFile file{};
	bool success = assetlib::load_binary_file(path, file);
	if (!success) {
		LOG_FORMAT(LogLevel::Warning, "Failed to open texture file {}", path);
		return;
	}

	assetlib::TextureInfo info = assetlib::read_texture_info(file);
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	gfx::Texture texture{};
	texture.image = ctx.create_image(ph::ImageType::Texture, { info.extents[0], info.extents[1] }, format);
	texture.view = ctx.create_image_view(texture.image);

	// Upload data into staging buffer
	uint32_t const size = get_full_image_byte_size(info.extents[0], info.extents[1], info.mip_levels, format);
	
	ph::RawBuffer staging = ctx.create_buffer(ph::BufferType::TransferBuffer, size);
	std::byte* memory = ctx.map_memory(staging);
	// Unpack and decompress directly into the staging buffer.
	assetlib::unpack_texture(info, file, memory);
	
	// Now copy from the staging buffer to the image
	ph::Queue& transfer = *ctx.get_queue(ph::QueueType::Transfer);
	ph::CommandBuffer cmd_buf = transfer.begin_single_time(thread);
	
	
	
	LOG_FORMAT(LogLevel::Info, "Loaded texture {}", path);
	LOG_FORMAT(LogLevel::Performance, "Texture {} (size {}x{} px) loaded with {} mip levels ({} MiB)",
		path, info.extents[0], info.extents[1], info.mip_levels, (float)size / (1024.0f * 1024.0f));
}

} // namespace impl
} // namespace andromeda