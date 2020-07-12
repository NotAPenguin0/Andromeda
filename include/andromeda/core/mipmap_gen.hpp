#ifndef ANDROMEDA_MIPMAP_GEN_HPP_
#define ANDROMEDA_MIMMAP_GEN_HPP_

#include <andromeda/core/context.hpp>
#include <phobos/core/vulkan_context.hpp>

namespace andromeda {

uint32_t get_mip_count(ph::RawImage const& image);

// Fills commands in cmd_buf to generate mipmaps for the specified image.
// Before calling this, the entire image must be in TransferSrcOptimal layout.
// After calling this, the entire image will be in layout specified by final_layout. The transition is done using a barrier
// with dstStageMask equal to dst_stage.
// cmd_buf must be a command buffer allocated from a pool with graphics capabilities.
void generate_mipmaps(vk::CommandBuffer cmd_buf, ph::RawImage& image, vk::ImageLayout final_layout, vk::AccessFlagBits dst_access, 
	vk::PipelineStageFlagBits dst_stage);

}

#endif