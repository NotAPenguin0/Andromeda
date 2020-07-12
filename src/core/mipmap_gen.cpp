#include <andromeda/core/mipmap_gen.hpp>

namespace andromeda {

uint32_t get_mip_count(ph::RawImage const& image) {
	return std::log2(std::max(image.size.width, image.size.height)) + 1;
}

void generate_mipmaps(vk::CommandBuffer cmd_buf, ph::RawImage& image, vk::ImageLayout final_layout, vk::AccessFlagBits dst_access, 
	vk::PipelineStageFlagBits dst_stage) {

	uint32_t const mip_count = get_mip_count(image);

	// Setup barrier data that is the same for all barriers that will be used
	vk::ImageMemoryBarrier barrier;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image.image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = image.layers;

	// We are now ready to generate the mip levels. Approach taken and adapted from Sasha Willems' Vulkan examples
	// https://github.com/SaschaWillems/Vulkan/blob/e370e6d169204bc3deaef637189336972414ffa5/examples/texturemipmapgen/texturemipmapgen.cpp#L264
	for (uint32_t mip = 1; mip < mip_count; ++mip) {
		vk::ImageBlit blit;
		// Source
		blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.srcSubresource.layerCount = image.layers;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.mipLevel = mip - 1;
		blit.srcOffsets[1].x = int32_t(image.size.width >> (mip - 1));
		blit.srcOffsets[1].y = int32_t(image.size.height >> (mip - 1));
		blit.srcOffsets[1].z = 1;
		// Destination
		blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blit.dstSubresource.layerCount = image.layers;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.mipLevel = mip;
		blit.dstOffsets[1].x = int32_t(image.size.width >> mip);
		blit.dstOffsets[1].y = int32_t(image.size.height >> mip);
		blit.dstOffsets[1].z = 1;

		vk::ImageSubresourceRange mip_sub_range;
		mip_sub_range.aspectMask = vk::ImageAspectFlagBits::eColor;
		mip_sub_range.baseMipLevel = mip;
		mip_sub_range.levelCount = 1;
		mip_sub_range.baseArrayLayer = 0;
		mip_sub_range.layerCount = image.layers;

		// Prepare current mip level as transfer destination
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.subresourceRange = mip_sub_range;
		barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);

		// Do the blit
		cmd_buf.blitImage(image.image, vk::ImageLayout::eTransferSrcOptimal,
			image.image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

		if (mip != mip_count - 1) {
			// Prepare current mip level as transfer source if it is not the last mip level
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);
		}
		else {
			// Else, transition to final layout immediately. We won't include the last mip level in the final barrier
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = dst_access;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = final_layout;
			cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, dst_stage, vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);
		}
	}
	
	// Transition the entire image except the final mip level to the final layout
	barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
	barrier.dstAccessMask = dst_access;
	barrier.subresourceRange.levelCount = std::max(mip_count - 1, 1u);
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
	barrier.newLayout = final_layout;

	cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, dst_stage, vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);

	image.current_layout = final_layout;
}

}