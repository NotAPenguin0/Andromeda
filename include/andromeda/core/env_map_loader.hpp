#ifndef ANDROMEDA_ENV_MAP_LOADER_HPP_
#define ANDROMEDA_ENV_MAP_LOADER_HPP_

#include <phobos/core/vulkan_context.hpp>
#include <phobos/renderer/command_buffer.hpp>
#include <phobos/present/frame_info.hpp>
#include <phobos/renderer/render_graph.hpp>
#include <andromeda/assets/env_map.hpp>
#include <andromeda/util/handle.hpp>
#include <string>
#include <ftl/task_scheduler.h>

namespace andromeda {

class Context;

struct EnvMapProcessData {
	static constexpr uint32_t cubemap_size = 2048;
	static constexpr uint32_t irradiance_map_size = 32;
	static constexpr uint32_t specular_map_base_size = 256;

	// Resources
	ph::RawImage raw_hdr;
	ph::ImageView hdr_view;
	ph::RawImage cube_map;
	ph::ImageView cubemap_view;
	ph::RawImage irradiance_map;
	ph::ImageView irradiance_map_view;
	ph::RawImage specular_map;
	ph::ImageView specular_map_view;

	std::vector<ph::ImageView> specular_map_mips;

	ph::RawBuffer upload_staging;

	// Synchronization primitives
	vk::Semaphore transfer_done;

	// Rendering data
	ph::FrameInfo fake_frame;
	vk::CommandBuffer raw_cmd_buf;
	std::unique_ptr<ph::CommandBuffer> cmd_buf;
};

struct EnvMapLoadInfo {
	Handle<EnvMap> handle;
	Context* context;
	std::string path;
};

class EnvMapLoader {
public:
	EnvMapLoader(ph::VulkanContext& vulkan);
	void load(ftl::TaskScheduler* scheduler, EnvMapLoadInfo load_info);
private:
	void create_projection_pipeline(ph::VulkanContext& vulkan);
	void create_convolution_pipeline(ph::VulkanContext& vulkan);
	void create_specular_prefilter_pipeline(ph::VulkanContext& vulkan);

	// Creates rendergraph, command buffer and fake frame
	void upload_hdr_image(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data,
		float* data, uint32_t width, uint32_t height);
	void begin_preprocess_commands(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data);
	void project_equirectangular_to_cubemap(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data);
	void create_irradiance_map(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data);
	void create_specular_map(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data);
	void end_preprocess_commands(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data);
	void cleanup(ftl::TaskScheduler* scheduler, ph::VulkanContext& vulkan, EnvMapProcessData& process_data);

	struct ProjectionBindings {
		ph::ShaderInfo::BindingInfo equirectangular;
		ph::ShaderInfo::BindingInfo cube_map;
	} projection_bindings;

	struct ConvolutionBindings {
		ph::ShaderInfo::BindingInfo cube_map;
		ph::ShaderInfo::BindingInfo irradiance_map;
	} convolution_bindings;

	struct SpecularPrefilterBindings {
		ph::ShaderInfo::BindingInfo cube_map;
		ph::ShaderInfo::BindingInfo specular_map;
	} specular_prefiter_bindings;

	vk::Sampler sampler;
};

}

#endif