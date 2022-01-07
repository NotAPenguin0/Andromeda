#include <andromeda/graphics/backend/forward_plus.hpp>

#include <glsl/limits.glsl>

using namespace std::literals::string_literals;

namespace andromeda::gfx::backend {

static constexpr float skybox_vertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    1.0f, -1.0f,  1.0f
};


ForwardPlusRenderer::ForwardPlusRenderer(gfx::Context& ctx) : RendererBackend(ctx) {
    for (int i = 0; i < gfx::MAX_VIEWPORTS; ++i) {
        // Note that only the main color and depth attachments need to be multisampled

        color_attachments[i] = gfx::Viewport::local_string(i, "forward_plus_color");
        ctx.create_attachment(color_attachments[i], {1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT, render_data.msaa_samples, ph::ImageType::ColorAttachment);

        depth_attachments[i] = gfx::Viewport::local_string(i, "forward_plus_depth");
        ctx.create_attachment(depth_attachments[i], { 1, 1 }, VK_FORMAT_D32_SFLOAT, render_data.msaa_samples, ph::ImageType::DepthStencilAttachment);

        heatmaps[i] = gfx::Viewport::local_string(i, "forward_plus_heatmap");
        ctx.create_attachment(heatmaps[i], {1, 1}, VK_FORMAT_R8G8B8A8_UNORM, ph::ImageType::StorageImage);

        // Create luminance histogram and average luminance buffers
        render_data.vp[i].luminance_histogram = ctx.create_buffer(ph::BufferType::StorageBufferStatic, ANDROMEDA_LUMINANCE_BINS * sizeof(uint32_t));
        render_data.vp[i].average_luminance = ctx.create_buffer(ph::BufferType::StorageBufferStatic, sizeof(float));

        ctx.name_object(render_data.vp[i].luminance_histogram.handle, gfx::Viewport::local_string(i, "Luminance Histogram"));
        ctx.name_object(render_data.vp[i].average_luminance.handle, gfx::Viewport::local_string(i, "Average Luminance"));
    }

    // Depth prepass pipeline
    {
        ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "depth_only")
            .add_shader("data/shaders/depth.vert.spv", "main", ph::ShaderStage::Vertex)
            .add_vertex_input(0)
                    // Note that not all these attributes will be used, but they are specified because the vertex size is deduced from them
            .add_vertex_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT) // iPos
            .add_vertex_attribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT) // iNormal
            .add_vertex_attribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT) // iTangent
            .add_vertex_attribute(0, 3, VK_FORMAT_R32G32_SFLOAT) // iUV
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .set_depth_test(true)
            .set_depth_write(true)
            .set_cull_mode(VK_CULL_MODE_BACK_BIT)
            .set_samples(render_data.msaa_samples)
            .set_sample_shading(render_data.msaa_sample_ratio)
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

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

    // Skybox pipeline
    {
        ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "skybox")
            .add_shader("data/shaders/skybox.vert.spv", "main", ph::ShaderStage::Vertex)
            .add_shader("data/shaders/skybox.frag.spv", "main", ph::ShaderStage::Fragment)
            .add_vertex_input(0)
            .add_vertex_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT) // iPos
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .set_depth_test(true)
            .set_depth_write(false)
            .set_depth_op(VK_COMPARE_OP_LESS_OR_EQUAL)
            .set_cull_mode(VK_CULL_MODE_NONE)
            .add_blend_attachment(false)
            .set_samples(render_data.msaa_samples)
            .set_sample_shading(render_data.msaa_sample_ratio)
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

    // Luminance calculation pipelines
    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "luminance_accumulate")
            .set_shader("data/shaders/luminance_accumulate.comp.spv", "main")
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }
    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "luminance_average")
            .set_shader("data/shaders/luminance_average.comp.spv", "main")
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

    // Tonemapping pipeline
    {
        ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "tonemap")
            .add_shader("data/shaders/tonemap.vert.spv", "main", ph::ShaderStage::Vertex)
            .add_shader("data/shaders/tonemap.frag.spv", "main", ph::ShaderStage::Fragment)
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .set_depth_test(false)
            .set_depth_write(false)
            .set_cull_mode(VK_CULL_MODE_NONE)
            .add_blend_attachment(false)
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }
}

ForwardPlusRenderer::~ForwardPlusRenderer() {
    for (auto& vp : render_data.vp) {
        ctx.destroy_buffer(vp.luminance_histogram);
        ctx.destroy_buffer(vp.average_luminance);
    }
}

void ForwardPlusRenderer::render_scene(ph::RenderGraph &graph, ph::InFlightContext &ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    // Create storage objects shared by the pipeline.
    create_render_data(ifc, viewport, scene);

    // Step 1 is to do a depth prepass of the entire scene. This will greatly increase efficiency of the final shading step by
    // eliminating pixel overdraw.
    graph.add_pass(depth_prepass(ifc, viewport, scene));
    // Step 2: a compute-based light culling pass that determines what part of the screen is covered by which lights, using the depth information from before.
    graph.add_pass(light_cull(ifc, viewport, scene));
    // Step 3: Apply shading
    graph.add_pass(shading(ifc, viewport, scene));
    // Step 4: Run the luminance accumulation pass to calculate the average luminance in the scene.
    // Note that this step together with the tonemapping step can be moved to another file in the future
    graph.add_pass(luminance_histogram(ifc, viewport, scene));
    // Step 5: Tonemap HDR color to final color attachment
    graph.add_pass(tonemap(ifc, viewport, scene));
}

std::vector<std::string> ForwardPlusRenderer::debug_views(gfx::Viewport viewport) {
    return { depth_attachments[viewport.index()], heatmaps[viewport.index()] };
}

void ForwardPlusRenderer::resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) {
    if (width != 0 && height != 0) {
        ctx.resize_attachment(color_attachments[viewport.index()], { width, height });
        ctx.resize_attachment(depth_attachments[viewport.index()], { width, height });
        ctx.resize_attachment(heatmaps[viewport.index()], { width, height });
    }
}

void ForwardPlusRenderer::create_render_data(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    auto& vp_data = render_data.vp[viewport.index()];

    // Camera data:
    //  - mat4 projection
    //  - mat4 pv
    //  - vec3 position (aligned to vec4)
    vp_data.camera = ifc.allocate_scratch_ubo(2 * sizeof(glm::mat4) + sizeof(glm::vec4));
    std::memcpy(vp_data.camera.data, &scene.cameras[viewport.index()].projection, sizeof(glm::mat4));
    std::memcpy(vp_data.camera.data + sizeof(glm::mat4), &scene.cameras[viewport.index()].proj_view, sizeof(glm::mat4));
    std::memcpy(vp_data.camera.data + 2 * sizeof(glm::mat4), &scene.cameras[viewport.index()].position, sizeof(glm::vec3));

    render_data.point_lights = ifc.allocate_scratch_ssbo(sizeof(gpu::PointLight) * scene.point_lights.size());
    std::memcpy(render_data.point_lights.data, scene.point_lights.data(), render_data.point_lights.range);

    // We can have at most MAX_LIGHTS in every tile, and there are ceil(W/TILE_SIZE) * ceil(H/TILE_SIZE) tiles, which gives us the size of the buffer
    vp_data.n_tiles_x = static_cast<uint32_t>(std::ceil((float)viewport.width() / ANDROMEDA_TILE_SIZE));
    vp_data.n_tiles_y = static_cast<uint32_t>(std::ceil((float)viewport.height() / ANDROMEDA_TILE_SIZE));
    uint32_t const n_tiles =  vp_data.n_tiles_x * vp_data.n_tiles_y;
    uint32_t const size = sizeof(uint32_t) * n_tiles * ANDROMEDA_MAX_LIGHTS_PER_TILE;
    vp_data.culled_lights = ifc.allocate_scratch_ssbo(size);
    std::memset(vp_data.culled_lights.data, 0, size);

    // Transform data
    render_data.transforms = ifc.allocate_scratch_ssbo(scene.draws.size() * sizeof(glm::mat4));
    for (int i = 0; i < scene.draws.size(); ++i) {
        render_data.transforms.data[i] = scene.draws[i].transform;
    }
}

ph::Pass ForwardPlusRenderer::depth_prepass(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    ph::Pass depth_pass = ph::PassBuilder::create("fwd_plus_depth")
        .add_depth_attachment(depth_attachments[viewport.index()], ph::LoadOp::Clear, {.depth_stencil = {.depth = 1.0f, .stencil = 0}})
        .execute([this, &ifc, &scene, viewport](ph::CommandBuffer& cmd) {
            cmd.bind_pipeline("depth_only");
            cmd.auto_viewport_scissor();

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_uniform_buffer("camera", render_data.vp[viewport.index()].camera)
                    .get();
            cmd.bind_descriptor_set(set);

            for (auto const& draw : scene.draws) {
                // Make sure to check for null meshes
                if (!draw.mesh) {
                    LOG_WRITE(LogLevel::Warning, "Draw with null mesh handle reached rendering system");
                    continue;
                }

                // Don't draw the mesh if it's not ready yet.
                if (!assets::is_ready(draw.mesh)) continue;
                // Don't draw if the material isn't ready yet.
                if (!assets::is_ready(draw.material)) continue;

                gfx::Mesh const& mesh = *assets::get(draw.mesh);

                cmd.bind_vertex_buffer(0, mesh.vertices);
                cmd.bind_index_buffer(mesh.indices, VK_INDEX_TYPE_UINT32);

                cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(glm::mat4), &draw.transform); // TODO: Use transforms SSBO like in main pass
                cmd.draw_indexed(mesh.num_indices, 1, 0, 0, 0);
            }
        })
        .get();
    return depth_pass;
}

ph::Pass ForwardPlusRenderer::light_cull(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
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

            uint32_t const pc[4] { viewport.width(), viewport.height(), vp_data.n_tiles_x, vp_data.n_tiles_y };
            cmd.push_constants(ph::ShaderStage::Compute, 0, 4 * sizeof(uint32_t), &pc);

            // Dispatch the compute shader with one thread group for every tile.
            cmd.dispatch(vp_data.n_tiles_x, vp_data.n_tiles_y, 1);
        })
    .get();
    return pass;
}

ph::Pass ForwardPlusRenderer::shading(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    auto& vp_data = render_data.vp[viewport.index()];
    ph::Pass pass = ph::PassBuilder::create("fdw_plus_shading")
        .add_attachment(color_attachments[viewport.index()], ph::LoadOp::Clear, ph::ClearValue{.color = {0.0, 0.0, 0.0, 1.0}})
        .add_depth_attachment(depth_attachments[viewport.index()], ph::LoadOp::Load, {})
        .shader_read_buffer(vp_data.culled_lights, ph::PipelineStage::FragmentShader)
        .execute([this, viewport, &vp_data, &scene, &ifc](ph::CommandBuffer& cmd) {
            Handle<gfx::Environment> env = scene.cameras[viewport.index()].environment;
            if (!env || !assets::is_ready(env)) env = scene.default_env;
            gfx::Environment* environment = assets::get(env);

            // We can skip drawing the scene if there are no draws.
            if (!scene.draws.empty()) {
                cmd.bind_pipeline("shading");
                cmd.auto_viewport_scissor();

                VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_info{};
                variable_count_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
                uint32_t counts[1]{MAX_TEXTURES};
                variable_count_info.descriptorSetCount = 1;
                variable_count_info.pDescriptorCounts = counts;

                Handle<gfx::Texture> brdf_lut_handle = scene.textures.brdf_lut;
                if (!brdf_lut_handle || !assets::is_ready(brdf_lut_handle))
                    brdf_lut_handle = scene.textures.default_metal_rough; // default metal-rough is black
                gfx::Texture *brdf_lut = assets::get(brdf_lut_handle);

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                        .add_uniform_buffer("camera", vp_data.camera)
                        .add_storage_buffer("lights", render_data.point_lights)
                        .add_storage_buffer("visible_lights", vp_data.culled_lights)
                        .add_storage_buffer("transforms", render_data.transforms)
                        .add_sampled_image("irradiance_map", environment->irradiance_view, ctx.basic_sampler())
                        .add_sampled_image("specular_map", environment->specular_view, ctx.basic_sampler())
                        .add_sampled_image("brdf_lut", brdf_lut->view, ctx.basic_sampler())
                        .add_sampled_image_array("textures", scene.textures.views, ctx.basic_sampler())
                        .add_pNext(&variable_count_info)
                        .get();
                cmd.bind_descriptor_set(set);

                for (uint32_t i = 0; i < scene.draws.size(); ++i) {
                    auto &draw = scene.draws[i];
                    // Make sure to check for null meshes
                    if (!draw.mesh) {
                        LOG_WRITE(LogLevel::Warning, "Draw with null mesh handle reached rendering system");
                        continue;
                    }

                    // Don't draw the mesh if it's not ready yet.
                    if (!assets::is_ready(draw.mesh)) continue;
                    // Don't draw if the material isn't ready yet.
                    if (!assets::is_ready(draw.material)) continue;

                    gfx::Mesh const &mesh = *assets::get(draw.mesh);
                    gfx::Material const &material = *assets::get(draw.material);

                    auto textures = scene.get_material_textures(draw.material);

                    cmd.bind_vertex_buffer(0, mesh.vertices);
                    cmd.bind_index_buffer(mesh.indices, VK_INDEX_TYPE_UINT32);

                    uint32_t const vtx_pc[]{
                            (uint32_t) i // Transform index
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

                    cmd.draw_indexed(mesh.num_indices, 1, 0, 0, 0);
                }
            }

            // Render the skybox (if there is one).
            // TODO: Later, we'll add an atmosphere shader that can be used/customized instead if the user wants
            {
                cmd.bind_pipeline("skybox");
                cmd.auto_viewport_scissor();
                ph::BufferSlice vbo = ifc.allocate_scratch_vbo(sizeof(skybox_vertices));
                std::memcpy(vbo.data, skybox_vertices, sizeof(skybox_vertices));
                cmd.bind_vertex_buffer(0, vbo);

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                        .add_sampled_image("skybox", environment->cubemap_view, ctx.basic_sampler())
                        .get();
                cmd.bind_descriptor_set(set);
                cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(glm::mat4), &scene.cameras[viewport.index()].projection);
                glm::mat4 view_no_position = glm::mat4(glm::mat3(scene.cameras[viewport.index()].view));
                cmd.push_constants(ph::ShaderStage::Vertex, sizeof(glm::mat4), sizeof(glm::mat4), &view_no_position);
                cmd.draw(36, 1, 0, 0);
            }
        })
        .get();
    return pass;
}

ph::Pass ForwardPlusRenderer::luminance_histogram(ph::InFlightContext &ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    auto& vp_data = render_data.vp[viewport.index()];
    ph::Pass pass = ph::PassBuilder::create_compute("luminance_histogram")
        .sample_attachment(color_attachments[viewport.index()], ph::PipelineStage::ComputeShader)
        .shader_write_buffer(vp_data.luminance_histogram, ph::PipelineStage::ComputeShader)
        .shader_read_buffer(vp_data.luminance_histogram, ph::PipelineStage::ComputeShader)
        .shader_write_buffer(vp_data.average_luminance, ph::PipelineStage::ComputeShader)
        .shader_read_buffer(vp_data.average_luminance, ph::PipelineStage::ComputeShader)
        .execute([this, &vp_data, &ifc, &scene, viewport](ph::CommandBuffer& cmd) {
            {
                cmd.bind_compute_pipeline("luminance_accumulate");

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                        .add_sampled_image("scene_hdr", ctx.get_attachment(color_attachments[viewport.index()]).view,
                                           ctx.basic_sampler())
                        .add_storage_buffer("histogram", vp_data.luminance_histogram)
                        .get();
                cmd.bind_descriptor_set(set);

                float const pc[2]{
                        scene.cameras[viewport.index()].min_log_luminance,
                        1.0f / (scene.cameras[viewport.index()].max_log_luminance -
                                scene.cameras[viewport.index()].min_log_luminance)
                };
                cmd.push_constants(ph::ShaderStage::Compute, 0, sizeof(pc), &pc);

                uint32_t const dispatches_x = std::ceil(
                        viewport.width() / (float) ANDROMEDA_LUMINANCE_ACCUMULATE_GROUP_SIZE);
                uint32_t const dispatches_y = std::ceil(
                        viewport.height() / (float) ANDROMEDA_LUMINANCE_ACCUMULATE_GROUP_SIZE);
                cmd.dispatch(dispatches_x, dispatches_y, 1);
            }

            // Add a barrier to protect the histogram buffer
            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = vp_data.luminance_histogram.handle;
            barrier.offset = 0;
            barrier.size = vp_data.luminance_histogram.size;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            cmd.barrier(ph::PipelineStage::ComputeShader, ph::PipelineStage::ComputeShader, barrier);

            {
                cmd.bind_compute_pipeline("luminance_average");

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_storage_buffer("average_luminance", vp_data.average_luminance)
                    .add_storage_buffer("histogram", vp_data.luminance_histogram)
                    .get();
                cmd.bind_descriptor_set(set);

                float const pc[4] {
                    scene.cameras[viewport.index()].min_log_luminance,
                    scene.cameras[viewport.index()].max_log_luminance - scene.cameras[viewport.index()].min_log_luminance,
                    ctx.delta_time(),
                    0.0 // padding
                };

                uint32_t const num_pixels = viewport.width() * viewport.height();

                cmd.push_constants(ph::ShaderStage::Compute, 0, 4 * sizeof(float), &pc);
                cmd.push_constants(ph::ShaderStage::Compute, 4 * sizeof(float), sizeof(uint32_t), &num_pixels);

                cmd.dispatch(1, 1, 1);
            }
        })
        .get();
    return pass;
}

ph::Pass ForwardPlusRenderer::tonemap(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    auto& vp_data = render_data.vp[viewport.index()];
    ph::Pass pass = ph::PassBuilder::create("forward_plus_tonemap")
        .add_attachment(viewport.target(), ph::LoadOp::Clear, ph::ClearValue{.color = {0.0, 0.0, 0.0, 1.0}})
        .sample_attachment(color_attachments[viewport.index()], ph::PipelineStage::FragmentShader)
        .shader_read_buffer(vp_data.average_luminance, ph::PipelineStage::FragmentShader)
        .execute([this, viewport, &vp_data](ph::CommandBuffer& cmd) {
            cmd.bind_pipeline("tonemap");
            cmd.auto_viewport_scissor();

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                .add_sampled_image("input_hdr", ctx.get_attachment(color_attachments[viewport.index()]).view, ctx.basic_sampler())
                .add_storage_buffer("average_luminance", vp_data.average_luminance)
                .get();

            cmd.bind_descriptor_set(set);

            cmd.push_constants(ph::ShaderStage::Fragment, 0, sizeof(uint32_t), &render_data.msaa_samples);

            cmd.draw(6, 1, 0, 0);
        })
        .get();

    return pass;
}

}