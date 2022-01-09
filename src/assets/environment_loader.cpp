#include <andromeda/assets/loaders.hpp>

#include <andromeda/graphics/context.hpp>

#include <assetlib/asset_file.hpp>
#include <assetlib/environment.hpp>

#include <string>

namespace andromeda::impl {

void load_environment(gfx::Context& ctx, Handle<gfx::Environment> handle, std::string_view path, uint32_t thread) {
    using namespace std::literals::string_literals;

    assetlib::AssetFile file{};
    plib::binary_input_stream stream = plib::binary_input_stream::from_file(path.data());
    bool success = assetlib::load_binary_file(stream, file);
    if (!success) {
        LOG_FORMAT(LogLevel::Error, "Failed to open environment file {}", path);
        return;
    }

    assetlib::EnvironmentInfo info = assetlib::read_environment_info(file);

    // Create one large staging buffer
    VkDeviceSize const total_size = info.hdr_bytes + info.irradiance_bytes + info.specular_bytes;
    ph::RawBuffer upload = ctx.create_buffer(ph::BufferType::TransferBuffer, total_size);
    std::byte* memory = ctx.map_memory(upload);
    // Unpack directly into staging memory
    assetlib::unpack_environment(info, file, memory, memory + info.hdr_bytes, memory + info.hdr_bytes + info.irradiance_bytes);

    // Create images and views
    gfx::Environment env{};
    env.cubemap = ctx.create_image(ph::ImageType::EnvMap, {info.hdr_extents[0], info.hdr_extents[1]},
                                   VK_FORMAT_R32G32B32A32_SFLOAT, std::log2(info.hdr_extents[0]) + 1);
    env.irradiance = ctx.create_image(ph::ImageType::EnvMap, {info.irradiance_size, info.irradiance_size}, VK_FORMAT_R32G32B32A32_SFLOAT);
    env.specular = ctx.create_image(ph::ImageType::EnvMap, {info.specular_size, info.specular_size},
                                    VK_FORMAT_R32G32B32A32_SFLOAT, std::log2(info.specular_size) + 1);
    env.cubemap_view = ctx.create_image_view(env.cubemap);
    env.irradiance_view = ctx.create_image_view(env.irradiance);
    env.specular_view = ctx.create_image_view(env.specular);

    ctx.name_object(env.cubemap, path.data() + " - Cubemap Image"s);
    ctx.name_object(env.irradiance, path.data() + " - Irradiance Image"s);
    ctx.name_object(env.specular, path.data() + " - Specular Image"s);

    ctx.name_object(env.cubemap_view, path.data() + " - Cubemap ImageView"s);
    ctx.name_object(env.irradiance_view, path.data() + " - Irradiance ImageView"s);
    ctx.name_object(env.specular_view, path.data() + " - Specular ImageView"s);

    // Record CPU->GPU copy for images
    // For this we first transition the images to TransferDstOptimal, then do the copy, then transfer to ShaderReadOnlyOptimal

    ph::Queue& queue = *ctx.get_queue(ph::QueueType::Transfer);
    ph::CommandBuffer cmd = queue.begin_single_time(thread);

    auto to_transfer_dst = [&ctx, &cmd](ph::ImageView view) {
        cmd.transition_layout(
            // Newly created image
            ph::PipelineStage::TopOfPipe, VK_ACCESS_NONE_KHR,
            // Next usage is the copy buffer to image command, so transfer and write access.
            ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
            // For the copy command to work the image needs to be in the TransferDstOptimal layout.
            view, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    };

    auto to_shader_read_only = [&ctx, &cmd](ph::ImageView view) {
        cmd.transition_layout(
            // Right after the copy operation
            ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
            // We don't use it anymore this submission
            ph::PipelineStage::BottomOfPipe, VK_ACCESS_MEMORY_READ_BIT,
            // Next usage will be a shader read operation.
            view, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
    };

    to_transfer_dst(env.cubemap_view);
    to_transfer_dst(env.irradiance_view);
    to_transfer_dst(env.specular_view);
    cmd.copy_buffer_to_image(upload.slice(0, info.hdr_bytes), env.cubemap_view);
    cmd.copy_buffer_to_image(upload.slice(info.hdr_bytes, info.irradiance_bytes), env.irradiance_view);
    cmd.copy_buffer_to_image(upload.slice(info.hdr_bytes + info.irradiance_bytes, info.specular_bytes), env.specular_view);
    to_shader_read_only(env.cubemap_view);
    to_shader_read_only(env.irradiance_view);
    to_shader_read_only(env.specular_view);

    VkFence fence = ctx.create_fence();
    queue.end_single_time(cmd, fence);
    ctx.wait_for_fence(fence);
    queue.free_single_time(cmd, thread);
    ctx.destroy_fence(fence);
    ctx.destroy_buffer(upload);

    assets::impl::make_ready(handle, env);

    // Log some info about the loaded environment
    LOG_FORMAT(LogLevel::Info, "Loaded environment {}", path);
    LOG_FORMAT(LogLevel::Performance, "Environment {} (face size {}x{} pixels) loaded with {} mip levels. ({:.1f} MiB). Irradiance resolution: {}px, Specular resolution: {}px",
               path, info.hdr_extents[0], info.hdr_extents[1], env.cubemap.mip_levels, (float) total_size / (1024.0f * 1024.0f), info.irradiance_size, info.specular_size);
}

}