#include <andromeda/graphics/context.hpp>

#include <thread>

namespace andromeda {
namespace gfx {

std::unique_ptr<Context> Context::init(Window& window, Log& logger) {
	ph::AppSettings settings{};
	settings.app_name = "Andromeda Scene Editor";
#if ANDROMEDA_DEBUG
	settings.enable_validation = true;
#endif
	// Use as many threads as the hardware supports
	settings.num_threads = std::thread::hardware_concurrency();
	// The Window and Log classes are subclasses of the proper interfaces so they can be used here.
	settings.wsi = &window;
	settings.logger = &logger;
	settings.gpu_requirements.dedicated = true;
	settings.gpu_requirements.min_video_memory = 2u * 1024u * 1024u * 1024u; // 2 GiB of VRAM minimum.
	settings.gpu_requirements.requested_queues = {
		ph::QueueRequest{.dedicated = false, .type = ph::QueueType::Graphics},
		ph::QueueRequest{.dedicated = true, .type = ph::QueueType::Transfer},
		ph::QueueRequest{.dedicated = true, .type = ph::QueueType::Compute}
	};
	settings.gpu_requirements.features.samplerAnisotropy = true;
	settings.gpu_requirements.features.fillModeNonSolid = true;
	settings.gpu_requirements.features.independentBlend = true;
	settings.gpu_requirements.features.shaderInt64 = true;
	settings.gpu_requirements.features_1_2.scalarBlockLayout = true;
	settings.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
	// Note that we cannot use make_unique since the constructor is private.
	auto ctx = std::unique_ptr<Context>{ new Context{ settings } };
	// Log some information about the created context
	LOG_FORMAT(LogLevel::Info, "Created graphics context.");
	ph::PhysicalDevice const& gpu = ctx->get_physical_device();
	LOG_FORMAT(LogLevel::Info, "Selected GPU: {}", gpu.properties.deviceName);
	uint64_t total_mem_size = 0;
	uint64_t device_local = 0;
	for (size_t i = 0; i < gpu.memory_properties.memoryHeapCount; ++i) {
		auto const& heap = gpu.memory_properties.memoryHeaps[i];
		total_mem_size += heap.size;
		if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
			device_local += heap.size;
		}
	}

	float mem_size_in_mb = (float)total_mem_size / (1024.0f * 1024.0f);
	float device_local_in_mb = (float)device_local / (1024.0f * 1024.0f);
	LOG_FORMAT(LogLevel::Performance, "Total available VRAM on device: {} MiB ({} MiB device local)", 
		mem_size_in_mb, device_local_in_mb);
	LOG_FORMAT(LogLevel::Performance, "Available hardware threads: {}", settings.num_threads);
	LOG_FORMAT(LogLevel::Performance, "Transfer queue is dedicated: {}", 
		ctx->get_queue(ph::QueueType::Transfer)->dedicated());
	LOG_FORMAT(LogLevel::Performance, "Compute queue is dedicated: {}", 
		ctx->get_queue(ph::QueueType::Compute)->dedicated());


	return ctx;
}

Context::Context(ph::AppSettings settings) : ph::Context(std::move(settings)) {

}

} // namespace gfx
} // namespace andromeda