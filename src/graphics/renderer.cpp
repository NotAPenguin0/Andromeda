#include <andromeda/graphics/renderer.hpp>

#include <andromeda/graphics/backend/forward_plus.hpp>
#include <andromeda/graphics/imgui.hpp>

#include <andromeda/graphics/environment.hpp>

#include <andromeda/components/transform.hpp>
#include <andromeda/components/mesh_renderer.hpp>
#include <andromeda/components/hierarchy.hpp>

#include <phobos/render_graph.hpp>

#include <andromeda/math/transform.hpp>

namespace andromeda::gfx {

/**
 * @brief Creates a 1x1 texture and returns a handle to it.
 *        May only be called from the main thread.
 * @param ctx Reference to the graphics context.
 * @param format Format of the texture image.
 * @param color Color to fill the single pixel of the image with.
 * @param size Size of the color buffer (in bytes). This isn't fixed so we can create different image formats with a single function.
 * @return A handle to the 1x1 texture in the asset system.
 */
static Handle<gfx::Texture> create_1x1_texture(gfx::Context& ctx, VkFormat format, uint8_t const* color, uint32_t size) {
    ph::RawImage image = ctx.create_image(ph::ImageType::Texture, {1, 1}, format);
    ph::ImageView view = ctx.create_image_view(image);

    // Fill image data. For this we'll need a command buffer. Since this function runs on the main thread thread_index is zero.
    ph::Queue& transfer = *ctx.get_queue(ph::QueueType::Transfer);
    ph::CommandBuffer cmd = transfer.begin_single_time(0);

    // Record transfer commands
    ph::RawBuffer staging = ctx.create_buffer(ph::BufferType::TransferBuffer, size);
    std::byte* memory = ctx.map_memory(staging);
    std::memcpy(memory, color, size);
    ctx.unmap_memory(staging);

    // Issue copy layout transitions and copy commands
    cmd.transition_layout(
        // Newly created image
        ph::PipelineStage::TopOfPipe, VK_ACCESS_NONE_KHR,
        // Next usage is the copy buffer to image command, so transfer and write access.
        ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
        // For the copy command to work the image needs to be in the TransferDstOptimal layout.
        view, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    cmd.copy_buffer_to_image(staging, view);
    cmd.transition_layout(
        // Right after the copy operation
        ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
        // We don't use it anymore this submission
        ph::PipelineStage::BottomOfPipe, VK_ACCESS_MEMORY_READ_BIT,
        // Next usage will be a shader read operation.
        view, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    VkFence fence = ctx.create_fence();
    transfer.end_single_time(cmd, fence);
    ctx.wait_for_fence(fence);
    ctx.destroy_fence(fence);
    transfer.free_single_time(cmd, 0);
    ctx.destroy_buffer(staging);

    gfx::Texture texture{
        .image = image,
        .view = view
    };

    return assets::take(texture);
}

static Handle<gfx::Environment> create_default_environment(gfx::Context& ctx) {
    gfx::Environment env{};

    env.cubemap = ctx.create_image(ph::ImageType::EnvMap, {1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT);
    env.irradiance = ctx.create_image(ph::ImageType::EnvMap, {1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT);
    env.specular = ctx.create_image(ph::ImageType::EnvMap, {1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT);
    env.cubemap_view = ctx.create_image_view(env.cubemap);
    env.irradiance_view = ctx.create_image_view(env.irradiance);
    env.specular_view = ctx.create_image_view(env.specular);

    ctx.name_object(env.cubemap, "Default Envmap - Image");
    ctx.name_object(env.irradiance, "Default Envmap Irradiance - Image");
    ctx.name_object(env.specular, "Default Envmap Specular - Image");
    ctx.name_object(env.cubemap_view, "Default Envmap - ImageView");
    ctx.name_object(env.irradiance_view, "Default Envmap Irradiance - ImageView");
    ctx.name_object(env.specular_view, "Default Envmap Specular - ImageView");

    ph::Queue& queue = *ctx.get_queue(ph::QueueType::Transfer);
    ph::CommandBuffer cmd = queue.begin_single_time(0);

    float data[4]{0.0f, 0.0f, 0.0f, 0.0f}; // zero alpha so this will never affect anything
    ph::RawBuffer upload = ctx.create_buffer(ph::BufferType::TransferBuffer, sizeof(data));
    std::byte* mem = ctx.map_memory(upload);
    std::memcpy(mem, data, sizeof(data));
    ctx.unmap_memory(upload);

    auto upload_image = [&ctx, &upload, &cmd](ph::ImageView img) {
        cmd.transition_layout(
            ph::PipelineStage::TopOfPipe, VK_ACCESS_NONE_KHR, ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
            img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        cmd.copy_buffer_to_image(upload, img);
        cmd.transition_layout(
            ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT, ph::PipelineStage::BottomOfPipe, VK_ACCESS_MEMORY_READ_BIT,
            img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
    };

    upload_image(env.cubemap_view);
    upload_image(env.irradiance_view);
    upload_image(env.specular_view);

    VkFence fence = ctx.create_fence();
    queue.end_single_time(cmd, fence);
    ctx.wait_for_fence(fence);
    ctx.destroy_fence(fence);
    queue.free_single_time(cmd, 0);
    ctx.destroy_buffer(upload);

    return assets::take(env);
}

Renderer::Renderer(gfx::Context& ctx, Window& window) {
    gfx::imgui::init(ctx, window);

    // Initialize viewports
    for (uint32_t i = 0; i < gfx::MAX_VIEWPORTS; ++i) {
        gfx::Viewport& vp = viewports[i].vp;
        vp.id = i;
        vp.target_name = gfx::Viewport::local_string(vp, "rendertarget");
        vp.width_px = window.width();
        vp.height_px = window.height();

        ctx.create_attachment(vp.target(), {vp.width(), vp.height()}, VK_FORMAT_R8G8B8A8_SRGB, ph::ImageType::ColorAttachment);
    }

    // Create default textures and add them to the scene
    {
        uint8_t magenta[4]{255, 0, 255, 255};
        uint8_t up[4]{128, 128, 255, 255};
        uint8_t arm[4]{0, 255, 0, 0};
        uint8_t white[1]{255};
        Handle<gfx::Texture> albedo = create_1x1_texture(ctx, VK_FORMAT_R8G8B8A8_SRGB, magenta, sizeof(magenta));
        Handle<gfx::Texture> normal = create_1x1_texture(ctx, VK_FORMAT_R8G8B8A8_UNORM, up, sizeof(up));
        Handle<gfx::Texture> metal_rough = create_1x1_texture(ctx, VK_FORMAT_R8G8B8A8_UNORM, arm, sizeof(arm));
        Handle<gfx::Texture> occlusion = create_1x1_texture(ctx, VK_FORMAT_R8_UNORM, white, sizeof(white));

        auto name_default_tex = [&ctx](Handle<gfx::Texture> t, std::string const& base) {
            gfx::Texture* tex = assets::get(t);
            ctx.name_object(tex->image, "default_" + base + " - image");
            ctx.name_object(tex->view, "default_" + base + " - view");
        };

        name_default_tex(albedo, "albedo");
        name_default_tex(normal, "normal");
        name_default_tex(metal_rough, "metal/rough");
        name_default_tex(occlusion, "occlusion");

        scene.set_default_albedo(albedo);
        scene.set_default_normal(normal);
        scene.set_default_metal_rough(metal_rough);
        scene.set_default_occlusion(occlusion);
    }

    scene.set_default_environment(create_default_environment(ctx));
    scene.set_brdf_lut(assets::load<gfx::Texture>("data/textures/brdf_lut.tx"));

    backend::DebugGeometryList::initialize(ctx);
    // register global pointer
    backend::impl::_debug_geometry_list_ptr = &debug_geometry;
    impl = std::make_unique<backend::ForwardPlusRenderer>(ctx);

    StatTracker::initialize(ctx);
    StatTracker::set_interval(60); // At 60 fps, this would update once every second.
}

void Renderer::shutdown(gfx::Context& ctx) {
    StatTracker::shutdown(ctx);
    gfx::imgui::shutdown();
    impl.reset(nullptr);
}

void Renderer::render_frame(gfx::Context& ctx, World const& world, bool dirty) {
    ph::InFlightContext ifc = ctx.wait_for_frame();

    // Reset scene description from last frame
    scene.reset();
    scene.set_dirty(dirty);

    for (auto const& viewport : viewports) {
        debug_geometry.clear(viewport.vp);
        DEBUG_SET_COLOR(viewport.vp, glm::vec3(1, 1, 1)); // Default color to white
    }

    StatTracker::new_frame(ctx);

    fill_scene_description(world);

    ph::Pass clear_swap = ph::PassBuilder::create("clear")
        .add_attachment(ctx.get_swapchain_attachment_name(),
                        ph::LoadOp::Clear, {.color = {0.0f, 0.0f, 0.0f, 1.0f}})
        .get();

    ph::RenderGraph graph{};
    graph.add_pass(std::move(clear_swap));

    impl->frame_setup(ifc, scene);
    for (auto const& viewport: viewports) {
        if (viewport.in_use) {
            impl->render_scene(graph, ifc, viewport.vp, scene);
            graph.add_pass(debug_geometry.build_render_pass(viewport.vp, ctx, ifc, scene.get_camera_info(viewport.vp).proj_view));
        }
    }

    // After all passes we add the imgui render pass
    imgui::render(graph, ifc, ctx.get_swapchain_attachment_name());

    graph.build(ctx);

    ph::RenderGraphExecutor executor{};
    ifc.command_buffer.begin();
    StatTracker::begin_query(ifc.command_buffer);
    executor.execute(ifc.command_buffer, graph);
    StatTracker::end_query(ifc.command_buffer);
    ifc.command_buffer.end();

    ctx.submit_frame_commands(*ctx.get_queue(ph::QueueType::Graphics), ifc.command_buffer, impl->wait_semaphores());
    ctx.present(*ctx.get_present_queue());
}

std::optional<gfx::Viewport> Renderer::create_viewport(uint32_t width, uint32_t height, ecs::entity_t camera) {
    // Find the first viewport slot that's not in use
    auto it = viewports.begin();
    while (it != viewports.end()) {
        // Found a free slot.
        if (!it->in_use) { break; }
        ++it;
    }

    // No free slots found
    if (it == viewports.end()) {
        return std::nullopt;
    }

    it->vp.camera_id = camera;
    it->in_use = true;
    return it->vp;
}

void Renderer::resize_viewport(gfx::Context& ctx, gfx::Viewport& viewport, uint32_t width, uint32_t height) {
    // Don't resize when one parameter is zero.
    if (width != 0 && height != 0) {
        ctx.resize_attachment(viewport.target(), {width, height});
        // Update viewport information
        viewport.width_px = width;
        viewport.height_px = height;

        viewports[viewport.index()].vp.width_px = width;
        viewports[viewport.index()].vp.height_px = height;

        impl->resize_viewport(viewport, width, height);
    }
}

std::vector<gfx::Viewport> Renderer::get_active_viewports() {
    std::vector<gfx::Viewport> result{};
    result.reserve(gfx::MAX_VIEWPORTS);
    for (auto const& viewport: viewports) {
        if (viewport.in_use) {
            result.push_back(viewport.vp);
        }
    }
    return result;
}

void Renderer::set_viewport_camera(gfx::Viewport& viewport, ecs::entity_t camera) {
    viewport.camera_id = camera;
    viewports[viewport.index()].vp.camera_id = camera;
}

void Renderer::destroy_viewport(gfx::Viewport viewport) {
    viewports[viewport.index()].in_use = false;
}

std::vector<std::string> Renderer::get_debug_views(gfx::Viewport viewport) {
    return impl->debug_views(viewport);
}

void Renderer::fill_scene_description(World const& world) {
    // Access the ECS.
    auto ecs = world.ecs();

    std::unordered_map<ecs::entity_t, glm::mat4> transform_lookup{};

    // Register all used materials
    for (auto[_, mesh]: ecs->view<Transform, MeshRenderer>()) {
        scene.add_material(mesh.material);
    }

    // Add all meshes in the world to the draw list
    for (auto[_, mesh, hierarchy]: ecs->view<Transform, MeshRenderer, Hierarchy>()) {
        glm::mat4 world_transform = math::local_to_world(hierarchy.this_entity, ecs, transform_lookup);
        scene.add_draw(mesh.mesh, mesh.material, mesh.occluder, world_transform);
    }

    // Add lighting information
    for (auto[_, light, hierarchy]: ecs->view<Transform, PointLight, Hierarchy>()) {
        glm::mat4 const world_transform = math::local_to_world(hierarchy.this_entity, ecs, transform_lookup);
        glm::vec3 const position = world_transform[3]; // Position is stored in the last column
        scene.add_light(light, position);
    }

    for (auto[transform, light, hierarchy]: ecs->view<Transform, DirectionalLight, Hierarchy>()) {
        // Directional lights do not respect parent rotations.
        glm::vec3 const rotation = transform.rotation;
        scene.add_light(light, rotation);
    }

    // Add every camera/viewport combo.
    for (auto const& viewport: viewports) {
        if (viewport.in_use) {
            ecs::entity_t camera = viewport.vp.camera();
            // Don't render viewports with no camera
            if (camera == ecs::no_entity) { continue; }
            scene.add_viewport(viewport.vp, ecs, camera);
        }
    }
}

} // namespace andromeda::gfx