#include <andromeda/graphics/backend/forward_plus.hpp>

using namespace std::literals::string_literals;

namespace andromeda::gfx::backend {

ForwardPlusRenderer::ForwardPlusRenderer(gfx::Context& ctx) : RendererBackend(ctx) {
    for (int i = 0; i < gfx::MAX_VIEWPORTS; ++i) {
        color_attachments[i] = gfx::Viewport::local_string(i, "forward_plus_color");
        ctx.create_attachment(color_attachments[i], {1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT, ph::ImageType::ColorAttachment);

        depth_attachments[i] = gfx::Viewport::local_string(i, "forward_plus_depth");
        ctx.create_attachment(depth_attachments[i], {1, 1}, VK_FORMAT_D32_SFLOAT, ph::ImageType::DepthStencilAttachment);

        heatmaps[i] = gfx::Viewport::local_string(i, "forward_plus_heatmap");
        ctx.create_attachment(heatmaps[i], {1, 1}, VK_FORMAT_R8G8B8A8_UNORM, ph::ImageType::StorageImage);
    }

    // Depth prepass pipeline
    {
        ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "depth_only")
            .add_shader("data/shaders/depth.vert.spv", "main", ph::ShaderStage::Vertex)
            .add_shader("data/shaders/depth.frag.spv", "main", ph::ShaderStage::Fragment)
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
    // Step 4: Tonemap HDR color to final color attachment
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

                cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(glm::mat4), &draw.transform);
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
        .write_storage_image(ctx.get_attachment(heatmaps[viewport.index()])->view, ph::PipelineStage::ComputeShader)
        .execute([this, &vp_data, &ifc, &scene, viewport](ph::CommandBuffer& cmd) {
            cmd.bind_compute_pipeline("light_cull");

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                .add_uniform_buffer("camera", vp_data.camera)
                .add_sampled_image("scene_depth", ctx.get_attachment(depth_attachments[viewport.index()])->view, ctx.basic_sampler())
                .add_storage_buffer("lights", render_data.point_lights)
                .add_storage_buffer("visible_lights", vp_data.culled_lights)
                .add_storage_image("light_heatmap", ctx.get_attachment(heatmaps[viewport.index()])->view)
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
            cmd.bind_pipeline("shading");
            cmd.auto_viewport_scissor();

            VkDescriptorSetVariableDescriptorCountAllocateInfo variable_count_info{};
            variable_count_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
            uint32_t counts[1]{ MAX_TEXTURES };
            variable_count_info.descriptorSetCount = 1;
            variable_count_info.pDescriptorCounts = counts;

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                .add_uniform_buffer("camera", vp_data.camera)
                .add_storage_buffer("lights", render_data.point_lights)
                .add_storage_buffer("visible_lights", vp_data.culled_lights)
                .add_storage_buffer("transforms", render_data.transforms)
                .add_sampled_image_array("textures", scene.textures.views, ctx.basic_sampler())
                .add_pNext(&variable_count_info)
                .get();
            cmd.bind_descriptor_set(set);

            for (uint32_t i = 0; i < scene.draws.size(); ++i) {
                auto& draw = scene.draws[i];
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
                gfx::Material const& material = *assets::get(draw.material);

                auto textures = scene.get_material_textures(draw.material);

                cmd.bind_vertex_buffer(0, mesh.vertices);
                cmd.bind_index_buffer(mesh.indices, VK_INDEX_TYPE_UINT32);

                uint32_t const vtx_pc[] {
                    (uint32_t)i // Transform index
                };

                uint32_t const frag_pc[] {
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
        })
        .get();
    return pass;
}

ph::Pass ForwardPlusRenderer::tonemap(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    ph::Pass pass = ph::PassBuilder::create("forward_plus_tonemap")
        .add_attachment(viewport.target(), ph::LoadOp::Clear, ph::ClearValue{.color = {0.0, 0.0, 0.0, 1.0}})
        .sample_attachment(color_attachments[viewport.index()], ph::PipelineStage::FragmentShader)
        .execute([this, viewport](ph::CommandBuffer& cmd) {
            cmd.bind_pipeline("tonemap");
            cmd.auto_viewport_scissor();

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                .add_sampled_image("input_hdr", ctx.get_attachment(color_attachments[viewport.index()])->view, ctx.basic_sampler())
                .get();

            cmd.bind_descriptor_set(set);

            cmd.draw(6, 1, 0, 0);
        })
        .get();

    return pass;
}

}