#include <andromeda/assets/loaders.hpp>

#include <andromeda/graphics/context.hpp>
#include <andromeda/graphics/texture.hpp>

#include <assetlib/asset_file.hpp>
#include <assetlib/texture.hpp>

#include <plib/stream.hpp>

#include <string>

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
	using namespace std::literals::string_literals;

	assetlib::AssetFile file{};
	plib::binary_input_stream in_stream = plib::binary_input_stream::from_file(path.data());
	bool success = assetlib::load_binary_file(in_stream, file);
	if (!success) {
		LOG_FORMAT(LogLevel::Error, "Failed to open texture file {}", path);
		return;
	}

	assetlib::TextureInfo info = assetlib::read_texture_info(file);
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	gfx::Texture texture{};
	texture.image = ctx.create_image(ph::ImageType::Texture, 
		{ info.extents[0], info.extents[1] }, format, info.mip_levels);
	texture.view = ctx.create_image_view(texture.image);

	ctx.name_object(texture.image.handle, path.data() + " - Image"s);
	ctx.name_object(texture.view, path.data() + " - ImageView"s);

	// Upload data into staging buffer
	uint32_t const size = get_full_image_byte_size(info.extents[0], info.extents[1], info.mip_levels, format);
	
	ph::RawBuffer staging = ctx.create_buffer(ph::BufferType::TransferBuffer, size);
	std::byte* memory = ctx.map_memory(staging);
	// Unpack and decompress directly into the staging buffer.
	assetlib::unpack_texture(info, file, memory);
	
	// Now copy from the staging buffer to the image
	ph::Queue& transfer = *ctx.get_queue(ph::QueueType::Transfer);
	ph::Queue& graphics = *ctx.get_queue(ph::QueueType::Graphics);
	ph::CommandBuffer cmd_buf = transfer.begin_single_time(thread);
	
	cmd_buf.transition_layout(
		// Newly created image
		ph::PipelineStage::TopOfPipe, VK_ACCESS_NONE_KHR,
		// Next usage is the copy buffer to image command, so tranfer and write access.
		ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
		// For the copy command to work the image needs to be in the TransferDstOptimal layout.
		texture.view, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	cmd_buf.copy_buffer_to_image(staging, texture.view);
	cmd_buf.transition_layout(
		// Right after the copy operation
		ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
		// We don't use it anymore this submission
		ph::PipelineStage::BottomOfPipe, VK_ACCESS_NONE_KHR,
		// Next usage will be a shader read operation.
		texture.view, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	// Now transfer ownership to the graphics queue. We additionally get a free transition.
	cmd_buf.release_ownership(transfer, graphics, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkFence fence = ctx.create_fence();
	VkSemaphore semaphore = ctx.create_semaphore();

	transfer.end_single_time(cmd_buf, nullptr, {}, nullptr, semaphore);

	ph::CommandBuffer graphics_cmd = graphics.begin_single_time(thread);
	graphics_cmd.acquire_ownership(transfer, graphics, texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	graphics.end_single_time(graphics_cmd, fence, ph::PipelineStage::TopOfPipe, semaphore);

	// Wait until the task is complete
	ctx.wait_for_fence(fence);

	// Cleanup all resources and send the image to the asset system

	ctx.destroy_buffer(staging);
	ctx.destroy_fence(fence);
	ctx.destroy_semaphore(semaphore);
	transfer.free_single_time(cmd_buf, thread);
	graphics.free_single_time(graphics_cmd, thread);

	assets::impl::make_ready(handle, std::move(texture));
	
	LOG_FORMAT(LogLevel::Info, "Loaded texture {}", path);
	LOG_FORMAT(LogLevel::Performance, "Texture {} (size {}x{} px) loaded with {} mip levels ({:.1f} MiB)",
		path, info.extents[0], info.extents[1], info.mip_levels, (float)size / (1024.0f * 1024.0f));
}

} // namespace impl
} // namespace andromeda