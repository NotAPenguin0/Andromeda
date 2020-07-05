#include <andromeda/core/env_map_loader.hpp>

#include <andromeda/assets/importers/stb_image.h>

#include <phobos/renderer/render_graph.hpp>
#include <phobos/renderer/render_attachment.hpp>
#include <andromeda/core/context.hpp>
#include <andromeda/assets/assets.hpp>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>


namespace andromeda {


EnvMapLoader::EnvMapLoader(ph::VulkanContext& ctx) {
	// Create pipelines for preprocessing the environment maps
	create_projection_pipeline(ctx);
	create_convolution_pipeline(ctx);
	create_specular_prefilter_pipeline(ctx);

	vk::SamplerCreateInfo sampler_info;
	sampler_info.magFilter = vk::Filter::eLinear;
	sampler_info.minFilter = vk::Filter::eLinear;
	sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
	sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
	sampler_info.anisotropyEnable = true;
	sampler_info.maxAnisotropy = 8;
	sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
	sampler_info.unnormalizedCoordinates = false;
	sampler_info.compareEnable = false;
	sampler_info.compareOp = vk::CompareOp::eAlways;
	sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler_info.minLod = 0.0;
	sampler_info.maxLod = 0.0;
	sampler_info.mipLodBias = 0.0;
	sampler = ctx.device.createSampler(sampler_info);
}

void EnvMapLoader::load(ftl::TaskScheduler* scheduler, EnvMapLoadInfo load_info) {
	Context* ctx = load_info.context;
	ph::VulkanContext& vulkan = *ctx->vulkan;

	EnvMapProcessData process_data;

	int w, h, channels;
	float* hdr_data = stbi_loadf(load_info.path.data(), &w, &h, &channels, 4);
	upload_hdr_image(scheduler, vulkan, process_data, hdr_data, w, h);
	begin_preprocess_commands(scheduler, vulkan, process_data);
	project_equirectangular_to_cubemap(scheduler, vulkan, process_data);
	create_irradiance_map(scheduler, vulkan, process_data);
	create_specular_map(scheduler, vulkan, process_data);
	end_preprocess_commands(scheduler, vulkan, process_data);
	cleanup(scheduler, vulkan, process_data);

	stbi_image_free(hdr_data);

	EnvMap result;
	result.cube_map = process_data.cube_map;
	result.cube_map_view = process_data.cubemap_view;
	result.irradiance_map = process_data.irradiance_map;
	result.irradiance_map_view = process_data.irradiance_map_view;
	result.specular_map = process_data.specular_map;
	result.specular_map_view = process_data.specular_map_view;

	assets::finalize_load(load_info.handle, std::move(result));
	io::log("Finished loading and preprocessing environment map {}", load_info.path);
}

void EnvMapLoader::upload_hdr_image(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data,
	float* data, uint32_t width, uint32_t height) {
	process_data.raw_hdr = ph::create_image(vulkan, width, height, ph::ImageType::HdrImage, vk::Format::eR32G32B32A32Sfloat);
	process_data.hdr_view = ph::create_image_view(vulkan.device, process_data.raw_hdr);
	
	vk::DeviceSize size = width * height * 4 * sizeof(float);
	process_data.upload_staging = ph::create_buffer(vulkan, size, ph::BufferType::TransferBuffer);
	std::byte* memory = ph::map_memory(vulkan, process_data.upload_staging);
	std::memcpy(memory, data, size);
	ph::unmap_memory(vulkan, process_data.upload_staging);
	
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();
	vk::CommandBuffer cmd_buf = vulkan.transfer->begin_single_time(thread_index);
	ph::transition_image_layout(cmd_buf, process_data.raw_hdr, vk::ImageLayout::eTransferDstOptimal);
	ph::copy_buffer_to_image(cmd_buf, ph::whole_buffer_slice(vulkan, process_data.upload_staging), process_data.raw_hdr);

	vulkan.transfer->release_ownership(cmd_buf, process_data.raw_hdr, *vulkan.compute);

	process_data.transfer_done = vulkan.device.createSemaphore({});
	vulkan.transfer->end_single_time(cmd_buf, nullptr, {}, nullptr, process_data.transfer_done);
}

void EnvMapLoader::begin_preprocess_commands(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();
	// First acquire ownership of the image, then exeucte transition to General so it can be used as a storage image
	vk::CommandBuffer cmd_buf = vulkan.compute->begin_single_time(thread_index);
	vulkan.compute->acquire_ownership(cmd_buf, process_data.raw_hdr, *vulkan.transfer);
	ph::transition_image_layout(cmd_buf, process_data.raw_hdr, vk::ImageLayout::eGeneral);

	process_data.raw_cmd_buf = cmd_buf;
	process_data.cmd_buf = std::make_unique<ph::CommandBuffer>(&vulkan, &process_data.fake_frame, &vulkan.thread_contexts[thread_index], cmd_buf);
}

void EnvMapLoader::project_equirectangular_to_cubemap(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	process_data.cube_map = ph::create_image(vulkan, process_data.cubemap_size, process_data.cubemap_size, 
		ph::ImageType::EnvMap, vk::Format::eR32G32B32A32Sfloat, 6);
	process_data.cubemap_view = ph::create_image_view(vulkan.device, process_data.cube_map);
	
	ph::transition_image_layout(process_data.raw_cmd_buf, process_data.cube_map, vk::ImageLayout::eGeneral);
	ph::CommandBuffer& cmd_buf = *process_data.cmd_buf;
	
	ph::Pipeline pipeline = cmd_buf.get_compute_pipeline("equirectangular_to_cubemap");
	cmd_buf.bind_pipeline(pipeline);
	
	ph::DescriptorSetBinding set_binding;
	set_binding.add(ph::make_descriptor(projection_bindings.equirectangular, process_data.hdr_view, sampler, vk::ImageLayout::eGeneral));
	set_binding.add(ph::make_descriptor(projection_bindings.cube_map, process_data.cubemap_view, vk::ImageLayout::eGeneral));

	vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
	cmd_buf.bind_descriptor_set(0, descr_set);

	cmd_buf.dispatch_compute(process_data.cubemap_size / 16, process_data.cubemap_size / 16, 6);
}

void EnvMapLoader::create_irradiance_map(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	process_data.irradiance_map = ph::create_image(vulkan, process_data.irradiance_map_size, process_data.irradiance_map_size,
		ph::ImageType::EnvMap, vk::Format::eR32G32B32A32Sfloat, 6);
	process_data.irradiance_map_view = ph::create_image_view(vulkan.device, process_data.irradiance_map);

	ph::transition_image_layout(process_data.raw_cmd_buf, process_data.irradiance_map, vk::ImageLayout::eGeneral);
	ph::CommandBuffer& cmd_buf = *process_data.cmd_buf;

	ph::Pipeline pipeline = cmd_buf.get_compute_pipeline("irradiance_map_convolution");
	cmd_buf.bind_pipeline(pipeline);

	// Insert barrier to make sure previous dispatch has completed before this one
	vk::ImageMemoryBarrier barrier;
	barrier.image = process_data.cube_map.image;
	barrier.oldLayout = process_data.cube_map.current_layout;
	barrier.newLayout = process_data.cube_map.current_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 6;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	cmd_buf.barrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, barrier);

	ph::DescriptorSetBinding set_binding;
	set_binding.add(ph::make_descriptor(convolution_bindings.cube_map, process_data.cubemap_view, sampler, vk::ImageLayout::eGeneral));
	set_binding.add(ph::make_descriptor(convolution_bindings.irradiance_map, process_data.irradiance_map_view, vk::ImageLayout::eGeneral));

	vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
	cmd_buf.bind_descriptor_set(0, descr_set);

	cmd_buf.dispatch_compute(process_data.irradiance_map_size / 16, process_data.irradiance_map_size / 16, 6);
}

void EnvMapLoader::create_specular_map(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	const uint32_t mip_count = std::log2(process_data.specular_map_base_size) + 1;

	process_data.specular_map = ph::create_image(vulkan, process_data.specular_map_base_size, process_data.specular_map_base_size,
		ph::ImageType::EnvMap, vk::Format::eR32G32B32A32Sfloat, 6, mip_count);
	process_data.specular_map_view = ph::create_image_view(vulkan.device, process_data.specular_map);

	process_data.specular_map_mips.resize(mip_count);
	for (uint32_t level = 0; level < mip_count; ++level) {
		process_data.specular_map_mips[level] = ph::create_image_view_level(vulkan.device, process_data.specular_map, level);
	}

	ph::transition_image_layout(process_data.raw_cmd_buf, process_data.specular_map, vk::ImageLayout::eGeneral);
	ph::CommandBuffer& cmd_buf = *process_data.cmd_buf;

	ph::Pipeline pipeline = cmd_buf.get_compute_pipeline("specular_map_prefilter");
	cmd_buf.bind_pipeline(pipeline);

	for (uint32_t level = 0; level < mip_count; ++level) {
		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(specular_prefiter_bindings.cube_map, process_data.cubemap_view, sampler, vk::ImageLayout::eGeneral));
		set_binding.add(ph::make_descriptor(specular_prefiter_bindings.specular_map, 
			process_data.specular_map_mips[level], vk::ImageLayout::eGeneral));
		vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
		cmd_buf.bind_descriptor_set(0, descr_set);

		float roughness = (float)level / (float)(mip_count - 1);
		cmd_buf.push_constants(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &roughness);

		uint32_t const mip_size = process_data.specular_map_base_size / pow(2, level);
		cmd_buf.dispatch_compute(std::max(mip_size / 32, 1u), std::max(mip_size / 32, 1u), 1);
	}
}

void EnvMapLoader::end_preprocess_commands(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();

	vulkan.compute->release_ownership(process_data.raw_cmd_buf, process_data.irradiance_map, *vulkan.graphics);
	vulkan.compute->release_ownership(process_data.raw_cmd_buf, process_data.cube_map, *vulkan.graphics);
	vulkan.compute->release_ownership(process_data.raw_cmd_buf, process_data.specular_map, *vulkan.graphics);

	vk::Fence fence = vulkan.device.createFence({});
	vulkan.compute->end_single_time(process_data.raw_cmd_buf, fence, vk::PipelineStageFlagBits::eTransfer, process_data.transfer_done);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());

	vk::CommandBuffer gfx_commands = vulkan.graphics->begin_single_time(thread_index);

	// Acquire ownership
	vulkan.graphics->acquire_ownership(gfx_commands, process_data.irradiance_map, *vulkan.compute);
	vulkan.graphics->acquire_ownership(gfx_commands, process_data.cube_map, *vulkan.compute);
	vulkan.graphics->acquire_ownership(gfx_commands, process_data.specular_map, *vulkan.compute);

	// Transition textures to ShaderReadOnlyOptimal
	ph::transition_image_layout(gfx_commands, process_data.irradiance_map, vk::ImageLayout::eShaderReadOnlyOptimal);
	ph::transition_image_layout(gfx_commands, process_data.cube_map, vk::ImageLayout::eShaderReadOnlyOptimal);
	ph::transition_image_layout(gfx_commands, process_data.specular_map, vk::ImageLayout::eShaderReadOnlyOptimal);
	
	vulkan.device.resetFences(fence);
	vulkan.graphics->end_single_time(gfx_commands, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	
	vulkan.graphics->free_single_time(gfx_commands, thread_index);
}

void EnvMapLoader::cleanup(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	ph::destroy_image_view(vulkan, process_data.hdr_view);
	ph::destroy_image(vulkan, process_data.raw_hdr);
	ph::destroy_buffer(vulkan, process_data.upload_staging);

	for (auto& mip : process_data.specular_map_mips) {
		ph::destroy_image_view(vulkan, mip);
	}

	vulkan.device.destroySemaphore(process_data.transfer_done);
	vulkan.compute->free_single_time(process_data.raw_cmd_buf, scheduler->GetCurrentThreadIndex());
}

void EnvMapLoader::create_projection_pipeline(ph::VulkanContext& ctx) {
	ph::ComputePipelineCreateInfo pci;

	pci.shader = ph::create_shader(ctx, ph::load_shader_code("data/shaders/equirect_to_cubemap.comp.spv"), "main", 
		vk::ShaderStageFlagBits::eCompute);
	ph::reflect_shaders(ctx, pci);
	projection_bindings.equirectangular = pci.shader_info["equirectangular"];
	projection_bindings.cube_map = pci.shader_info["cube_map"];

	ctx.pipelines.create_named_pipeline("equirectangular_to_cubemap", std::move(pci));
}

void EnvMapLoader::create_convolution_pipeline(ph::VulkanContext& ctx) {
	ph::ComputePipelineCreateInfo pci;
	
	pci.shader = ph::create_shader(ctx, ph::load_shader_code("data/shaders/irradiance_convolution.comp.spv"), "main",
		vk::ShaderStageFlagBits::eCompute);
	ph::reflect_shaders(ctx, pci);
	convolution_bindings.cube_map = pci.shader_info["cube_map"];
	convolution_bindings.irradiance_map = pci.shader_info["irradiance_map"];

	ctx.pipelines.create_named_pipeline("irradiance_map_convolution", std::move(pci));
}

void EnvMapLoader::create_specular_prefilter_pipeline(ph::VulkanContext& ctx) {
	ph::ComputePipelineCreateInfo pci;

	pci.shader = ph::create_shader(ctx, ph::load_shader_code("data/shaders/prefilter_specular_map.comp.spv"), "main",
		vk::ShaderStageFlagBits::eCompute);
	ph::reflect_shaders(ctx, pci);
	specular_prefiter_bindings.cube_map = pci.shader_info["cube_map"];
	specular_prefiter_bindings.specular_map = pci.shader_info["specular_map"];

	ctx.pipelines.create_named_pipeline("specular_map_prefilter", std::move(pci));
}

}