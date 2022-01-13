#include <andromeda/graphics/context.hpp>

#include <andromeda/assets/loaders.hpp>

namespace andromeda {
namespace gfx {

std::unique_ptr<Context> Context::init(Window& window, Log& logger, thread::TaskScheduler& scheduler) {
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
    settings.gpu_requirements.features.sampleRateShading = true; // MSAA
    settings.gpu_requirements.features.depthClamp = true;
    settings.gpu_requirements.features.pipelineStatisticsQuery = true;
    settings.gpu_requirements.features.fragmentStoresAndAtomics = true;
    settings.gpu_requirements.features_1_2.scalarBlockLayout = true;
    settings.gpu_requirements.features_1_2.bufferDeviceAddress = true;
    // Enable descriptor indexing as this is the method we'll use to bind all our textures.
    settings.gpu_requirements.features_1_2.descriptorIndexing = true;
    settings.gpu_requirements.features_1_2.descriptorBindingPartiallyBound = true;
    settings.gpu_requirements.features_1_2.descriptorBindingVariableDescriptorCount = true;
    settings.gpu_requirements.features_1_2.runtimeDescriptorArray = true;
    settings.scratch_ibo_size = 1024 * 1024;
    settings.scratch_vbo_size = 1024 * 1024;
    settings.scratch_ssbo_size = 32 * 1024 * 1024; // 32 MiB
    settings.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    // Note that we cannot use make_unique since the constructor is private.
    auto ctx = std::unique_ptr<Context>{new Context{settings, window, scheduler}};
    // Log some information about the created context
    LOG_WRITE(LogLevel::Info, "Created graphics context.");
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

    float mem_size_in_mb = (float) total_mem_size / (1024.0f * 1024.0f);
    float device_local_in_mb = (float) device_local / (1024.0f * 1024.0f);
    LOG_FORMAT(LogLevel::Performance, "Total available VRAM on device: {:.1f} MiB ({:.1f} MiB device local)",
               mem_size_in_mb, device_local_in_mb);
    LOG_FORMAT(LogLevel::Performance, "Available hardware threads: {}", settings.num_threads);
    LOG_FORMAT(LogLevel::Performance, "Transfer queue is dedicated: {}",
               ctx->get_queue(ph::QueueType::Transfer)->dedicated());
    LOG_FORMAT(LogLevel::Performance, "Compute queue is dedicated: {}",
               ctx->get_queue(ph::QueueType::Compute)->dedicated());


    return ctx;
}

float Context::delta_time() const {
    return window.delta_time();
}

Context::Context(ph::AppSettings settings, Window& window, thread::TaskScheduler& scheduler)
    : ph::Context(std::move(settings)), window(window), scheduler(scheduler) {


}

Handle<gfx::Texture> Context::request_texture(std::string const& path) {
    LOG_FORMAT(LogLevel::Info, "Loading texture at path {}.", path);
    Handle<gfx::Texture> handle = assets::impl::insert_pending<gfx::Texture>();
    thread::task_id task = scheduler.schedule([this, handle, path](uint32_t thread) {
        try {
            impl::load_texture(*this, handle, path, thread + 1);
        }
        catch (std::exception const& e) {
            LOG_FORMAT(LogLevel::Error, "Exception while loading texture {}: {}", path, e.what());
        }
    });
    // Store load task so we can give the unload task a proper dependency.
    assets::impl::set_load_task(handle, task);
    assets::impl::set_path(handle, path);
    return handle;
}

Handle<gfx::Mesh> Context::request_mesh(std::string const& path) {
    LOG_FORMAT(LogLevel::Info, "Loading mesh at path {}", path);
    Handle<gfx::Mesh> handle = assets::impl::insert_pending<gfx::Mesh>();
    thread::task_id task = scheduler.schedule([this, handle, path](uint32_t thread) {
        try {
            impl::load_mesh(*this, handle, path, thread + 1);
        }
        catch (std::exception const& e) {
            LOG_FORMAT(LogLevel::Error, "Exception while loading mesh {}: {}", path, e.what());
        }
    });
    assets::impl::set_load_task(handle, task);
    assets::impl::set_path(handle, path);
    return handle;
}

Handle<gfx::Material> Context::request_material(std::string const& path) {
    LOG_FORMAT(LogLevel::Info, "Loading material at path {}", path);
    Handle<gfx::Material> handle = assets::impl::insert_pending<gfx::Material>();
    scheduler.schedule([this, handle, path](uint32_t thread) {
        try {
            impl::load_material(*this, handle, path, thread + 1);
        }
        catch (std::exception const& e) {
            LOG_FORMAT(LogLevel::Error, "Exception while loading material {}: {}", path, e.what());
        }
    });

    // No need to set load task, as materials don't get unloaded explicitly.
    assets::impl::set_path(handle, path);
    return handle;
}

Handle<gfx::Environment> Context::request_environment(std::string const& path) {
    LOG_FORMAT(LogLevel::Info, "Loading environment at path {}", path);
    Handle<gfx::Environment> handle = assets::impl::insert_pending<gfx::Environment>();
    thread::task_id task = scheduler.schedule([this, handle, path](uint32_t thread) {
        try {
            impl::load_environment(*this, handle, path, thread + 1);
        } catch (std::exception const& e) {
            LOG_FORMAT(LogLevel::Error, "Exception while loading environment {}: {}", path, e.what());
        }
    }, {});
    assets::impl::set_load_task(handle, task);
    assets::impl::set_path(handle, path);
    return handle;
}

void Context::free_texture(Handle<gfx::Texture> handle) {
    try {
        // If we free a texture before it is fully loaded we will get an error, unless
        // we set the load task as a dependency of the unload task
        thread::task_id dependency = assets::impl::get_load_task(handle);
        // Note that we only want to add the task as a dependency if there was a loading task.
        // This can happen when a resource was loaded synchronously.
        std::vector<thread::task_id> dependencies;
        if (dependency != static_cast<thread::task_id>(-1)) {
            dependencies.push_back(dependency);
        }
        scheduler.schedule([this, handle](uint32_t thread) {
            gfx::Texture* tex = assets::get(handle);
            if (tex == nullptr) {
                LOG_WRITE(LogLevel::Error, "Tried to delete null texture");
                return;
            }
            destroy_image_view(tex->view);
            destroy_image(tex->image);
            // Remove the asset from the asset system.
            assets::impl::delete_asset(handle);
        }, dependencies);
    }
    catch (std::exception const& e) {
        LOG_FORMAT(LogLevel::Fatal, "Fatal exception occurred. Provided message: {}", e.what());
        std::terminate();
    }
}

void Context::free_mesh(Handle<gfx::Mesh> handle) {
    try {
        // If we free a mesh before it is fully loaded we will get an error, unless
        // we set the load task as a dependency of the unload task
        thread::task_id dependency = assets::impl::get_load_task(handle);
        // Note that we only want to add the task as a dependency if there was a loading task.
        // This can happen when a resource was loaded synchronously.
        std::vector<thread::task_id> dependencies;
        if (dependency != static_cast<thread::task_id>(-1)) {
            dependencies.push_back(dependency);
        }
        scheduler.schedule([this, handle](uint32_t thread) {
            gfx::Mesh* mesh = assets::get(handle);
            if (mesh == nullptr) {
                LOG_WRITE(LogLevel::Error, "Tried to delete null mesh");
                return;
            }
            destroy_buffer(mesh->vertices);
            destroy_buffer(mesh->indices);
            // Remove the asset from the asset system.
            assets::impl::delete_asset(handle);
        }, dependencies);
    }
    catch (std::exception const& e) {
        LOG_FORMAT(LogLevel::Fatal, "Fatal exception occurred. Provided message: {}", e.what());
        std::terminate();
    }
}

void Context::free_environment(Handle<gfx::Environment> handle) {
    try {
        thread::task_id dependency = assets::impl::get_load_task(handle);
        std::vector<thread::task_id> dependencies;
        if (dependency != static_cast<thread::task_id>(-1)) {
            dependencies.push_back(dependency);
        }
        scheduler.schedule([this, handle](uint32_t thread) {
            gfx::Environment* env = assets::get(handle);
            if (env == nullptr) {
                LOG_WRITE(LogLevel::Error, "Tried to delete null environment");
                return;
            }
            destroy_image(env->cubemap);
            destroy_image(env->irradiance);
            destroy_image(env->specular);
            destroy_image_view(env->cubemap_view);
            destroy_image_view(env->irradiance_view);
            destroy_image_view(env->specular_view);
            assets::impl::delete_asset(handle);
        }, dependencies);
    }
    catch (std::exception const& e) {
        LOG_FORMAT(LogLevel::Fatal, "Fatal exception occurred. Provided message: {}", e.what());
        std::terminate();
    }
}

} // namespace gfx
} // namespace andromeda