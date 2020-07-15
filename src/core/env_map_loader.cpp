#include <andromeda/core/env_map_loader.hpp>

#include <andromeda/assets/importers/stb_image.h>

#include <phobos/renderer/render_graph.hpp>
#include <phobos/renderer/render_attachment.hpp>
#include <andromeda/core/context.hpp>
#include <andromeda/core/mipmap_gen.hpp>
#include <andromeda/assets/assets.hpp>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>


namespace andromeda {

struct ComputeGroupSize {
	uint32_t x = 0, y = 0, z = 0;
};

struct ComputeDispatchOffset {
	uint32_t x = 0, y = 0, z = 0;
};

struct PumpDispatchGroup {
	// Vulkan context, used to create fences and wait for them
	ph::VulkanContext* vulkan;
	// Thread index, necessary to correctly allocate command buffers
	uint32_t thread_index = 0;
	// Queue to allocate command buffers from. Must have compute support.
	ph::Queue* queue;
	// Maximum amount of concurrently running dispatches. More means the GPU will be busier.
	uint32_t max_concurrent_dispatches = 0;
	// Maximum amount of dispatches per second. 0 for unlimited.
	uint32_t max_dispatches_per_second = 0;
	// Minimum amount of workgroups in a single dispatch
	ComputeGroupSize min_workgroups;
	// Maximum amount of workgroups in a single dispatch
	ComputeGroupSize max_workgroups;
	// Local size of the compute shader
	ComputeGroupSize local_size;
	// Total amount of work groups to be processed
	ComputeGroupSize workgroups;

	// Function to be called on each dispatch. ComputeGroupSize is the amount the work groups that must be fed into
	// vkCmdDispatch. ComputeDispatchOffset is current offset.
	std::function<void(ph::CommandBuffer&, ComputeGroupSize, ComputeDispatchOffset)> dispatch_func;
};

vk::CommandBuffer launch_pump_dispatch(PumpDispatchGroup& dispatch_data, ComputeGroupSize workgroups, 
	ComputeDispatchOffset offset, vk::Fence fence, ph::FrameInfo& fake_frame) {
	// Create command buffer
	vk::CommandBuffer cmd_buf = dispatch_data.queue->begin_single_time(dispatch_data.thread_index);
	ph::CommandBuffer cbuf(dispatch_data.vulkan, &fake_frame, &dispatch_data.vulkan->thread_contexts[dispatch_data.thread_index], cmd_buf);

	// Dispatch
	dispatch_data.dispatch_func(cbuf, workgroups, offset);

	dispatch_data.queue->end_single_time(cmd_buf, fence);
	return cmd_buf;
}

ComputeDispatchOffset next_dispatch_offset(ComputeDispatchOffset current, ComputeGroupSize dispatch_size, ComputeGroupSize total_size) {
	ComputeDispatchOffset offset = current;
	// If these values are true for x, y and z, we are at the end and need to return the total_size as offset
	bool wrapped[3]{ false, false, false };

	offset.x += dispatch_size.x;
	if (offset.x >= total_size.x) {
		wrapped[0] = true;
		offset.x = 0;
		offset.y += dispatch_size.y;
	}
	if (offset.y >= total_size.y) {
		wrapped[1] = true;
		offset.y = 0;
		offset.z += dispatch_size.z;
	}
	if (offset.z >= total_size.z) {
		wrapped[2] = true;
		offset.z = total_size.z;
	}

	// see comment above
	if (wrapped[0] && wrapped[1] && wrapped[2]) {
		return { total_size.x, total_size.y, total_size.z };
	}

	return offset;
}

// Pumps out compute dispatches at a fixed rate to prevent filling the GPU with work.
// After calling this function, no extra synchronization is required as it blocks until all dispatches are completed.
void pump_dispatch(PumpDispatchGroup dispatch) {
	struct DispatchData {
		vk::Fence fence;
		vk::CommandBuffer cmd_buf;
	};

	std::vector<DispatchData> running_dispatches;

	// TODO: possibly design a better algorithm for choosing the ideal workgroup size
	ComputeGroupSize const workgroup_size = {
		.x = std::min(dispatch.max_workgroups.x, dispatch.workgroups.x),
		.y = std::min(dispatch.max_workgroups.y, dispatch.workgroups.y),
		.z = std::min(dispatch.max_workgroups.z, dispatch.workgroups.z)
	};
	ComputeGroupSize const total_size{
		.x = dispatch.workgroups.x * dispatch.local_size.x,
		.y = dispatch.workgroups.y * dispatch.local_size.y,
		.z = dispatch.workgroups.z * dispatch.local_size.z
	};

	ComputeGroupSize dispatch_size{ 
		.x = workgroup_size.x * dispatch.local_size.x,
		.y = workgroup_size.y * dispatch.local_size.y,
		.z = workgroup_size.z * dispatch.local_size.z 
	};
	ComputeDispatchOffset current_offset{ 0, 0, 0 };

	// One shared fake frame to minimize descriptor set reallocation
	ph::FrameInfo fake_frame;

	// To prevent the system from launching new dispatches while the final ones are stopping
	bool stop_launch = false;

	uint64_t ms_elapsed = 0; // This only counts the timeouts (for now?)
	uint64_t dispatches_this_second = 0;
	while (true) {
		// If there are no more dispatches running and there are no more new dispatches to be started, break out of the infinite loop.
		if (current_offset.x == total_size.x && current_offset.y == total_size.y && current_offset.z == total_size.z) {
			stop_launch = true;
			if (running_dispatches.empty()) { break; }
		}

		// Launch new dispatches if there is space
		if (dispatch.max_dispatches_per_second != 0 && dispatches_this_second >= dispatch.max_dispatches_per_second) {
			io::log("Max dispatches per second of {} exceeded! Rate-limiting", dispatch.max_dispatches_per_second);
			stop_launch = true;
		}

		if (!stop_launch && running_dispatches.size() < dispatch.max_concurrent_dispatches) {
			for (uint32_t i = running_dispatches.size(); i < dispatch.max_concurrent_dispatches; ++i) {
				vk::Fence fence = dispatch.vulkan->device.createFence({});
				vk::CommandBuffer cmd_buf = launch_pump_dispatch(dispatch, workgroup_size, current_offset, fence, fake_frame);
				current_offset = next_dispatch_offset(current_offset, dispatch_size, total_size);
				running_dispatches.push_back({ fence, cmd_buf });
				dispatches_this_second += 1;
			}
		}
		// Check status of running dispatches

		// Timeout of 1 ms for each wait. Timeout is measured in nanoseconds.
		constexpr uint64_t max_timeout = 1000000;
		for (auto it = running_dispatches.begin(); it != running_dispatches.end(); ++it) {
			vk::Result result = dispatch.vulkan->device.waitForFences(it->fence, true, max_timeout);
			// Dispatch completed, remove it from the list so a new one can be launched
			if (result == vk::Result::eSuccess) {
				dispatch.vulkan->device.destroyFence(it->fence);
				dispatch.queue->free_single_time(it->cmd_buf, dispatch.thread_index);
				it = running_dispatches.erase(it);
				// This erase trick can cause an end iterator to be incremented if the last element was erased, so we need this extra check.
				if (it == running_dispatches.end()) { break; }
			} else if (result == vk::Result::eTimeout) {
				ms_elapsed += max_timeout / 1000000; // nanoseconds to milliseconds
			}
		}

		// If a second has elapsed, reset max dispatch counter
		if (ms_elapsed > 1000) {
			ms_elapsed = 0;
			dispatches_this_second = 0;
			stop_launch = false;
		}
	}
}

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
	sampler_info.maxAnisotropy = 16;
	sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
	sampler_info.unnormalizedCoordinates = false;
	sampler_info.compareEnable = false;
	sampler_info.compareOp = vk::CompareOp::eAlways;
	sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler_info.minLod = 0.0;
	sampler_info.maxLod = 64;
	sampler_info.mipLodBias = 0.0;
	sampler = ctx.device.createSampler(sampler_info);

	sampler_info.maxLod = 0.0;
	no_mip_sampler = ctx.device.createSampler(sampler_info);
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
	generate_cube_mipmap(scheduler, vulkan, process_data);
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

	vk::Fence fence = vulkan.device.createFence({});
	vulkan.transfer->end_single_time(cmd_buf, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	vulkan.device.destroyFence(fence);
}

void EnvMapLoader::begin_preprocess_commands(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();

	// First acquire ownership of the HDR image, then exeucte transition to General so it can be used as a storage image
	vk::CommandBuffer cmd_buf = vulkan.compute->begin_single_time(thread_index);
	vulkan.compute->acquire_ownership(cmd_buf, process_data.raw_hdr, *vulkan.transfer);
	ph::transition_image_layout(cmd_buf, process_data.raw_hdr, vk::ImageLayout::eGeneral);

	// Create cube map and transition to general layout
	process_data.cube_map = ph::create_image(vulkan, process_data.cubemap_size, process_data.cubemap_size,
		ph::ImageType::EnvMap, vk::Format::eR32G32B32A32Sfloat, 6, fast_log2(process_data.cubemap_size) + 1);
	process_data.cubemap_view = ph::create_image_view(vulkan.device, process_data.cube_map);

	ph::transition_image_layout(cmd_buf, process_data.cube_map, vk::ImageLayout::eGeneral);

	vk::Fence fence = vulkan.device.createFence({});
	vulkan.compute->end_single_time(cmd_buf, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	vulkan.device.destroyFence(fence);
}

void EnvMapLoader::project_equirectangular_to_cubemap(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	PumpDispatchGroup pump;
	pump.local_size = { 16, 16, 1 };
	pump.workgroups = { process_data.cubemap_size / 16, process_data.cubemap_size / 16, 6 };
	pump.max_workgroups = { 16, 16, 1 };
	pump.min_workgroups = { 4, 4, 1 };
	pump.max_concurrent_dispatches = 1;
	pump.queue = vulkan.compute.get();
	pump.vulkan = &vulkan;
	pump.thread_index = scheduler->GetCurrentThreadIndex();
	pump.dispatch_func = [this, &process_data](ph::CommandBuffer& cmd_buf, ComputeGroupSize workgroups, ComputeDispatchOffset offset) {
		ph::Pipeline pipeline = cmd_buf.get_compute_pipeline("equirectangular_to_cubemap");
		cmd_buf.bind_pipeline(pipeline);

		cmd_buf.push_constants(vk::ShaderStageFlagBits::eCompute, 0, 3 * sizeof(uint32_t), &offset);

		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(projection_bindings.equirectangular, process_data.hdr_view, sampler, vk::ImageLayout::eGeneral));
		set_binding.add(ph::make_descriptor(projection_bindings.cube_map, process_data.cubemap_view, vk::ImageLayout::eGeneral));

		vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
		cmd_buf.bind_descriptor_set(0, descr_set);

		cmd_buf.dispatch_compute(workgroups.x, workgroups.y, workgroups.z);
	};
	pump_dispatch(pump);
	

	// Release ownership to graphics queue
	vk::CommandBuffer cmd_buf = vulkan.compute->begin_single_time(scheduler->GetCurrentThreadIndex());
	vulkan.compute->release_ownership(cmd_buf, process_data.cube_map, *vulkan.graphics);
	vk::Fence fence = vulkan.device.createFence({});
	vulkan.compute->end_single_time(cmd_buf, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	vulkan.device.destroyFence(fence);
}

void EnvMapLoader::generate_cube_mipmap(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();
	vk::CommandBuffer cmd_buf = vulkan.graphics->begin_single_time(thread_index);
	vulkan.graphics->acquire_ownership(cmd_buf, process_data.cube_map, *vulkan.compute);

	// Transition entire image to TransferSrcOptimal
	vk::ImageMemoryBarrier barrier;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = process_data.cube_map.image;

	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = process_data.cube_map.layers;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = get_mip_count(process_data.cube_map);
	
	barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

	barrier.oldLayout = vk::ImageLayout::eGeneral;
	barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	
	cmd_buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, 
		vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);

	generate_mipmaps(cmd_buf, process_data.cube_map, vk::ImageLayout::eGeneral, vk::AccessFlagBits::eShaderRead, 
		vk::PipelineStageFlagBits::eComputeShader);

	vulkan.graphics->release_ownership(cmd_buf, process_data.cube_map, *vulkan.compute);
	vk::Fence fence = vulkan.device.createFence({});
	vulkan.graphics->end_single_time(cmd_buf, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	vulkan.device.destroyFence(fence);
}

void EnvMapLoader::create_irradiance_map(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	process_data.irradiance_map = ph::create_image(vulkan, process_data.irradiance_map_size, process_data.irradiance_map_size,
		ph::ImageType::EnvMap, vk::Format::eR32G32B32A32Sfloat, 6);
	process_data.irradiance_map_view = ph::create_image_view(vulkan.device, process_data.irradiance_map);

	vk::CommandBuffer cmd_buf = vulkan.compute->begin_single_time(scheduler->GetCurrentThreadIndex());
	ph::transition_image_layout(cmd_buf, process_data.irradiance_map, vk::ImageLayout::eGeneral);
	vk::Fence fence = vulkan.device.createFence({});
	vulkan.compute->end_single_time(cmd_buf, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	vulkan.device.destroyFence(fence);

	io::log("Starting irradiance pump");
	PumpDispatchGroup pump;
	pump.local_size = { 16, 16, 1 };
	pump.workgroups = { process_data.irradiance_map_size / 16, process_data.irradiance_map_size / 16, 6 };
	pump.max_workgroups = { 4, 4, 1 };
	pump.min_workgroups = { 4, 4, 1 };
	pump.max_concurrent_dispatches = 1;
	pump.queue = vulkan.compute.get();
	pump.vulkan = &vulkan;
	pump.thread_index = scheduler->GetCurrentThreadIndex();
	pump.dispatch_func = [this, &process_data](ph::CommandBuffer& cmd_buf, ComputeGroupSize workgroups, ComputeDispatchOffset offset) {
		io::log("dispatch launched with offset x = {}, y = {}, z = {}", offset.x, offset.y, offset.z);
		ph::Pipeline pipeline = cmd_buf.get_compute_pipeline("irradiance_map_convolution");
		cmd_buf.bind_pipeline(pipeline);

		cmd_buf.push_constants(vk::ShaderStageFlagBits::eCompute, 0, 3 * sizeof(uint32_t), &offset);

		ph::DescriptorSetBinding set_binding;
		set_binding.add(ph::make_descriptor(convolution_bindings.cube_map, process_data.cubemap_view, no_mip_sampler, vk::ImageLayout::eGeneral));
		set_binding.add(ph::make_descriptor(convolution_bindings.irradiance_map, process_data.irradiance_map_view, vk::ImageLayout::eGeneral));

		vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
		cmd_buf.bind_descriptor_set(0, descr_set);

		cmd_buf.dispatch_compute(workgroups.x, workgroups.y, workgroups.z);
	};
	pump_dispatch(pump);
}

void EnvMapLoader::create_specular_map(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	constexpr uint32_t mip_count = fast_log2(process_data.specular_map_base_size) + 1;

	process_data.specular_map = ph::create_image(vulkan, process_data.specular_map_base_size, process_data.specular_map_base_size,
		ph::ImageType::EnvMap, vk::Format::eR32G32B32A32Sfloat, 6, mip_count);
	process_data.specular_map_view = ph::create_image_view(vulkan.device, process_data.specular_map);

	process_data.specular_map_mips.resize(mip_count);
	for (uint32_t level = 0; level < mip_count; ++level) {
		process_data.specular_map_mips[level] = ph::create_image_view_level(vulkan.device, process_data.specular_map, level);
	}

	vk::CommandBuffer cmd_buf = vulkan.compute->begin_single_time(scheduler->GetCurrentThreadIndex());
	ph::transition_image_layout(cmd_buf, process_data.specular_map, vk::ImageLayout::eGeneral);
	vk::Fence fence = vulkan.device.createFence({});
	vulkan.compute->end_single_time(cmd_buf, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	vulkan.device.destroyFence(fence);

	for (uint32_t level = 0; level < mip_count; ++level) {
		io::log("Starting pump for miplevel {}", level);
		PumpDispatchGroup pump;
		pump.local_size = { 16, 16, 1 };
		uint32_t const mip_size = process_data.specular_map_base_size / pow(2, level);
		pump.workgroups = { std::max(mip_size / 16, 1u), std::max(mip_size / 16, 1u), 6 };
		pump.max_workgroups = { 4, 4, 1 };
		pump.min_workgroups = { 4, 4, 1 };
		pump.max_concurrent_dispatches = 1;
		pump.max_dispatches_per_second = 512;
		pump.queue = vulkan.compute.get();
		pump.vulkan = &vulkan;
		pump.thread_index = scheduler->GetCurrentThreadIndex();
		pump.dispatch_func = [this, &process_data, level](ph::CommandBuffer& cmd_buf, ComputeGroupSize workgroups, ComputeDispatchOffset offset) {
			ph::Pipeline pipeline = cmd_buf.get_compute_pipeline("specular_map_prefilter");
			cmd_buf.bind_pipeline(pipeline);

			ph::DescriptorSetBinding set_binding;
			set_binding.add(ph::make_descriptor(specular_prefiter_bindings.cube_map, process_data.cubemap_view, 
				sampler, vk::ImageLayout::eGeneral));
			set_binding.add(ph::make_descriptor(specular_prefiter_bindings.specular_map,
				process_data.specular_map_mips[level], vk::ImageLayout::eGeneral));

			vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
			cmd_buf.bind_descriptor_set(0, descr_set);

			float roughness = std::clamp((float)level / (float)(mip_count - 1), 0.04f, 1.0f);
			cmd_buf.push_constants(vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &roughness);
			cmd_buf.push_constants(vk::ShaderStageFlagBits::eCompute, sizeof(float), 3 * sizeof(uint32_t), &offset);

			cmd_buf.dispatch_compute(workgroups.x, workgroups.y, workgroups.z);
		};
		pump_dispatch(pump);
	}
}

void EnvMapLoader::end_preprocess_commands(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data) {
	uint32_t const thread_index = scheduler->GetCurrentThreadIndex();

	vk::CommandBuffer cmd_buf = vulkan.compute->begin_single_time(thread_index);
	vulkan.compute->release_ownership(cmd_buf, process_data.irradiance_map, *vulkan.graphics);
	vulkan.compute->release_ownership(cmd_buf, process_data.cube_map, *vulkan.graphics);
	vulkan.compute->release_ownership(cmd_buf, process_data.specular_map, *vulkan.graphics);

	vk::Fence fence = vulkan.device.createFence({});
	vulkan.compute->end_single_time(cmd_buf, fence);
	vulkan.device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
	vulkan.compute->free_single_time(cmd_buf, thread_index);
	vulkan.device.destroyFence(fence);

	vk::CommandBuffer gfx_commands = vulkan.graphics->begin_single_time(thread_index);

	// Acquire ownership
	vulkan.graphics->acquire_ownership(gfx_commands, process_data.irradiance_map, *vulkan.compute);
	vulkan.graphics->acquire_ownership(gfx_commands, process_data.cube_map, *vulkan.compute);
	vulkan.graphics->acquire_ownership(gfx_commands, process_data.specular_map, *vulkan.compute);

	// Transition textures to ShaderReadOnlyOptimal
	ph::transition_image_layout(gfx_commands, process_data.irradiance_map, vk::ImageLayout::eShaderReadOnlyOptimal);
	ph::transition_image_layout(gfx_commands, process_data.cube_map, vk::ImageLayout::eShaderReadOnlyOptimal);
	ph::transition_image_layout(gfx_commands, process_data.specular_map, vk::ImageLayout::eShaderReadOnlyOptimal);
	
	fence = vulkan.device.createFence({});
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