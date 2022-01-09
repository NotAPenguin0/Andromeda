#include <andromeda/graphics/backend/forward_plus.hpp>
#include <andromeda/graphics/backend/depth_pass.hpp>
#include <andromeda/graphics/backend/mesh_draw.hpp>
#include <andromeda/graphics/backend/skybox.hpp>
#include <andromeda/graphics/backend/tonemap.hpp>

#include <andromeda/util/memory.hpp>

#include <glsl/limits.glsl>

using namespace std::literals::string_literals;

namespace andromeda::gfx::backend {


ForwardPlusRenderer::ForwardPlusRenderer(gfx::Context& ctx) : RendererBackend(ctx), render_data(ctx) {
    for (int i = 0; i < gfx::MAX_VIEWPORTS; ++i) {
        // Note that only the main color and depth attachments need to be multisampled

        color_attachments[i] = gfx::Viewport::local_string(i, "forward_plus_color");
        ctx.create_attachment(color_attachments[i], {1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT, render_data.msaa_samples, ph::ImageType::ColorAttachment);

        depth_attachments[i] = gfx::Viewport::local_string(i, "forward_plus_depth");
        ctx.create_attachment(depth_attachments[i], {1, 1}, VK_FORMAT_D32_SFLOAT, render_data.msaa_samples, ph::ImageType::DepthStencilAttachment);

        heatmaps[i] = gfx::Viewport::local_string(i, "forward_plus_heatmap");
        ctx.create_attachment(heatmaps[i], {1, 1}, VK_FORMAT_R8G8B8A8_UNORM, ph::ImageType::StorageImage);

        // Create average luminance buffer
        render_data.vp[i].average_luminance = ctx.create_buffer(ph::BufferType::StorageBufferStatic, sizeof(float));
        ctx.name_object(render_data.vp[i].average_luminance.handle, gfx::Viewport::local_string(i, "Average Luminance"));
    }

    create_depth_only_pipeline(ctx, render_data.msaa_samples, render_data.msaa_sample_ratio);
    create_skybox_pipeline(ctx, render_data.msaa_samples, render_data.msaa_sample_ratio);
    create_luminance_histogram_pipelines(ctx);
    create_tonemapping_pipeline(ctx);

    // Light culling shader
    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "light_cull")
            .set_shader("data/shaders/light_cull.comp.spv", "main")
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

    // Final shading pipeline
    {
        ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "shading")
            .add_shader("data/shaders/shading.vert.spv", "main", ph::ShaderStage::Vertex)
            .add_shader("data/shaders/shading.frag.spv", "main", ph::ShaderStage::Fragment)
            .add_vertex_input(0)
            .add_vertex_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT) // iPos
            .add_vertex_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT) // iNormal
            .add_vertex_attribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT) // iTangent
            .add_vertex_attribute(0, 3, VK_FORMAT_R32G32_SFLOAT) // iUV
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .set_depth_test(true)
            .set_depth_op(VK_COMPARE_OP_LESS_OR_EQUAL)
            .set_depth_write(false) // Do not write to the depth buffer since we already have depth information
            .set_cull_mode(VK_CULL_MODE_BACK_BIT)
            .add_blend_attachment(false)
            .set_samples(render_data.msaa_samples)
            .set_sample_shading(render_data.msaa_sample_ratio)
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }
}

ForwardPlusRenderer::~ForwardPlusRenderer() {
    for (auto& vp: render_data.vp) {
        ctx.destroy_buffer(vp.average_luminance);
    }
}

void ForwardPlusRenderer::frame_setup(ph::InFlightContext& ifc, gfx::SceneDescription const& scene) {
    // TODO: Move common code for filling scratch buffers to function?

    // Render data that's shared across all viewports.
    auto const point_lights = scene.get_point_lights();
    render_data.point_lights = ifc.allocate_scratch_ssbo(util::memsize(point_lights));
    std::memcpy(render_data.point_lights.data, point_lights.data(), util::memsize(point_lights));

    auto const dir_lights = scene.get_directional_lights();
    render_data.dir_lights = ifc.allocate_scratch_ssbo(util::memsize(dir_lights));
    std::memcpy(render_data.dir_lights.data, dir_lights.data(), util::memsize(dir_lights));

    auto const transforms = scene.get_draw_transforms();
    render_data.transforms = ifc.allocate_scratch_ssbo(util::memsize(transforms));
    std::memcpy(render_data.transforms.data, transforms.data(), util::memsize(transforms));

    // Update scene acceleration structure for RT.
    render_data.accel_structure.update(scene);
}

void ForwardPlusRenderer::render_scene(ph::RenderGraph& graph, ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    // Create storage objects shared by the pipeline.
    create_render_data(ifc, viewport, scene);

    auto const& vp = render_data.vp[viewport.index()];

    // Step 1 is to do a depth prepass of the entire scene. This will greatly increase efficiency of the final shading step by
    // eliminating pixel overdraw.
    graph.add_pass(build_depth_pass(ctx, ifc, depth_attachments[viewport.index()], scene, vp.camera, render_data.transforms));
    // Step 2: a compute-based light culling pass that determines what part of the screen is covered by which lights, using the depth information from before.
    graph.add_pass(light_cull(ifc, viewport, scene));
    // Step 3: Apply shading
    graph.add_pass(shading(ifc, viewport, scene));
    // Step 4: Run the luminance accumulation pass to calculate the average luminance in the scene.
    graph.add_pass(build_average_luminance_pass(ctx, ifc, color_attachments[viewport.index()], viewport, scene, vp.average_luminance));
    // Step 5: Tonemap HDR color to final color attachment
    graph.add_pass(build_tonemap_pass(ctx, color_attachments[viewport.index()], viewport.target(), render_data.msaa_samples, vp.average_luminance));
}

std::vector<std::string> ForwardPlusRenderer::debug_views(gfx::Viewport viewport) {
    std::vector<std::string> result = {depth_attachments[viewport.index()], heatmaps[viewport.index()]};
    return result;
}

std::vector<ph::WaitSemaphore> ForwardPlusRenderer::wait_semaphores() {
    // Our main rendering pass must wait on the TLAS build to be completed.
    VkSemaphore semaphore = render_data.accel_structure.get_tlas_build_semaphore();
    // Do not actually wait if the TLAS contained no instances (and is therefore empty).
    if (render_data.accel_structure.get_acceleration_structure() == nullptr) { return {}; }
    return {
        // Wait in the fragment shader. Note that we might need to update this as time goes by and we use
        // the acceleration structure in more stages.
        ph::WaitSemaphore{.handle = semaphore, .stage_flags = ph::PipelineStage::FragmentShader}
    };
}

void ForwardPlusRenderer::resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) {
    if (width != 0 && height != 0) {
        ctx.resize_attachment(color_attachments[viewport.index()], {width, height});
        ctx.resize_attachment(depth_attachments[viewport.index()], {width, height});
        ctx.resize_attachment(heatmaps[viewport.index()], {width, height});
    }
}

void ForwardPlusRenderer::create_render_data(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    auto& vp_data = render_data.vp[viewport.index()];

    auto const camera = scene.get_camera_info(viewport);

    // Camera data:
    //  - mat4 projection
    //  - mat4 view
    //  - mat4 pv
    //  - vec3 position (aligned to vec4)
    vp_data.camera = ifc.allocate_scratch_ubo(3 * sizeof(glm::mat4) + sizeof(glm::vec4));
    std::memcpy(vp_data.camera.data, &camera.projection, sizeof(glm::mat4));
    std::memcpy(vp_data.camera.data + sizeof(glm::mat4), &camera.view, sizeof(glm::mat4));
    std::memcpy(vp_data.camera.data + 2 * sizeof(glm::mat4), &camera.proj_view, sizeof(glm::mat4));
    std::memcpy(vp_data.camera.data + 3 * sizeof(glm::mat4), &camera.position, sizeof(glm::vec3));

    // We can have at most MAX_LIGHTS in every tile, and there are ceil(W/TILE_SIZE) * ceil(H/TILE_SIZE) tiles, which gives us the size of the buffer
    vp_data.n_tiles_x = static_cast<uint32_t>(std::ceil((float) viewport.width() / ANDROMEDA_TILE_SIZE));
    vp_data.n_tiles_y = static_cast<uint32_t>(std::ceil((float) viewport.height() / ANDROMEDA_TILE_SIZE));
    uint32_t const n_tiles = vp_data.n_tiles_x * vp_data.n_tiles_y;
    uint32_t const size = sizeof(uint32_t) * n_tiles * ANDROMEDA_MAX_LIGHTS_PER_TILE;
    vp_data.culled_lights = ifc.allocate_scratch_ssbo(size);
}

ph::Pass ForwardPlusRenderer::light_cull(ph::InFlightContext& ifc, gfx::Viewport const& viewport, gfx::SceneDescription const& scene) {
    auto& vp_data = render_data.vp[viewport.index()];
    ph::Pass pass = ph::PassBuilder::create_compute("fwd_plus_light_cull")
        .sample_attachment(depth_attachments[viewport.index()], ph::PipelineStage::ComputeShader)
        .shader_write_buffer(vp_data.culled_lights, ph::PipelineStage::ComputeShader)
        .write_storage_image(ctx.get_attachment(heatmaps[viewport.index()]).view, ph::PipelineStage::ComputeShader)
        .execute([this, &vp_data, &ifc, &scene, viewport](ph::CommandBuffer& cmd) {
            cmd.bind_compute_pipeline("light_cull");

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                .add_uniform_buffer("camera", vp_data.camera)
                .add_sampled_image("scene_depth", ctx.get_attachment(depth_attachments[viewport.index()]).view, ctx.basic_sampler())
                .add_storage_buffer("lights", render_data.point_lights)
                .add_storage_buffer("visible_lights", vp_data.culled_lights)
                .add_storage_image("light_heatmap", ctx.get_attachment(heatmaps[viewport.index()]).view)
                .get();
            cmd.bind_descriptor_set(set);

            uint32_t const pc[4]{viewport.width(), viewport.height(), vp_data.n_tiles_x, vp_data.n_tiles_y};
            cmd.push_constants(ph::ShaderStage::Compute, 0, 4 * sizeof(uint32_t), &pc);

            // Dispatch the compute shader with one thread group for every tile.
            cmd.dispatch(vp_data.n_tiles_x, vp_data.n_tiles_y, 1);
        })
        .get();
    return pass;
}

ph::Pass ForwardPlusRenderer::shading(ph::InFlightContext& ifc, gfx::Viewport const& viewport, gfx::SceneDescription const& scene) {
    auto& vp_data = render_data.vp[viewport.index()];
    ph::PassBuilder builder = ph::PassBuilder::create("fdw_plus_shading")
        .add_attachment(color_attachments[viewport.index()], ph::LoadOp::Clear, ph::ClearValue{.color = {0.0, 0.0, 0.0, 1.0}})
        .add_depth_attachment(depth_attachments[viewport.index()], ph::LoadOp::Load, {})
        .shader_read_buffer(vp_data.culled_lights, ph::PipelineStage::FragmentShader);

    ph::Pass pass = builder.execute([this, viewport, &vp_data, &scene, &ifc](ph::CommandBuffer& cmd) {
            Handle <gfx::Environment> env = scene.get_camera_info(viewport).environment;
            if (!env || !assets::is_ready(env)) { env = scene.get_default_environment(); }
            gfx::Environment const& environment = *assets::get(env);

            VkAccelerationStructureKHR tlas = render_data.accel_structure.get_acceleration_structure();

            // We can skip drawing the scene if there are no draws.
            // This happens when the scene drawlist is empty, OR when the TLAS is completely empty (in which case its handle is NULL).
            if (!scene.get_draws().empty() && tlas != nullptr) {
                cmd.bind_pipeline("shading");
                cmd.auto_viewport_scissor();

                VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_info{};
                variable_count_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
                uint32_t counts[1]{MAX_TEXTURES};
                variable_count_info.descriptorSetCount = 1;
                variable_count_info.pDescriptorCounts = counts;

                Handle <gfx::Texture> brdf_lut_handle = scene.get_brdf_lookup();
                if (!brdf_lut_handle || !assets::is_ready(brdf_lut_handle)) {
                    brdf_lut_handle = scene.get_default_albedo();
                }
                gfx::Texture* brdf_lut = assets::get(brdf_lut_handle);

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_uniform_buffer("camera", vp_data.camera)
                    .add_storage_buffer("lights", render_data.point_lights)
                    .add_storage_buffer("dir_lights", render_data.dir_lights)
                    .add_storage_buffer("visible_lights", vp_data.culled_lights)
                    .add_storage_buffer("transforms", render_data.transforms)
                    .add_sampled_image("irradiance_map", environment.irradiance_view, ctx.basic_sampler())
                    .add_sampled_image("specular_map", environment.specular_view, ctx.basic_sampler())
                    .add_sampled_image("brdf_lut", brdf_lut->view, ctx.basic_sampler())
                    .add_acceleration_structure("scene_tlas", tlas)
                    .add_sampled_image_array("textures", scene.get_textures(), ctx.basic_sampler())
                    .add_pNext(&variable_count_info)
                    .get();
                cmd.bind_descriptor_set(set);

                // Loop over each mesh, check if it's ready and if so render it
                for_each_ready_mesh(scene, [&scene, &vp_data, &cmd](auto const& draw, gfx::Mesh const& mesh, uint32_t index) {
                    // Don't draw if the material isn't ready yet.
                    if (!assets::is_ready(draw.material)) { return; }
                    gfx::Material const& material = *assets::get(draw.material);

                    auto textures = scene.get_material_textures(draw.material);

                    uint32_t const vtx_pc[]{
                        (uint32_t) index // Transform index
                    };

                    uint32_t const frag_pc[]{
                        vp_data.n_tiles_x,
                        vp_data.n_tiles_y,
                        textures.albedo,
                        textures.normal,
                        textures.metal_rough,
                        textures.occlusion
                    };

                    cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(uint32_t), vtx_pc);
                    // Note: Due to padding rules in the PC block the transform index in the vertex shader is followed by 4 bytes of padding.
                    cmd.push_constants(ph::ShaderStage::Fragment, 2 * sizeof(uint32_t), 6 * sizeof(uint32_t), frag_pc);

                    bind_and_draw(cmd, mesh);
                });
            }

            // Render the skybox (if there is one).
            if (env != scene.get_default_environment()) {
                render_skybox(ctx, ifc, cmd, environment, scene.get_camera_info(viewport));
            }
        })
        .get();
    return pass;
}

}