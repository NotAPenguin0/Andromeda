#include <andromeda/graphics/backend/forward_plus.hpp>

using namespace std::literals::string_literals;

namespace andromeda::gfx::backend {

ForwardPlusRenderer::ForwardPlusRenderer(gfx::Context& ctx) : RendererBackend(ctx) {
    for (int i = 0; i < gfx::MAX_VIEWPORTS; ++i) {
        depth_attachments[i] = gfx::Viewport::local_string(i, "forward_plus_depth");
        ctx.create_attachment(depth_attachments[i], {1, 1}, VK_FORMAT_D32_SFLOAT);
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
}

void ForwardPlusRenderer::render_scene(ph::RenderGraph &graph, ph::InFlightContext &ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    // Create storage objects shared by the pipeline.
    create_render_data(ifc, viewport, scene);

    // Step 1 is to do a depth prepass of the entire scene. This will greatly increase efficiency of the final shading step by
    // eliminating pixel overdraw.
    graph.add_pass(depth_prepass(ifc, viewport, scene));
    // Step 2: a compute-based light culling pass that determines what part of the screen is covered by which lights, using the depth information from before.
    graph.add_pass(light_cull(ifc, viewport, scene));
}

std::vector<std::string> ForwardPlusRenderer::debug_views(gfx::Viewport viewport) {
    return { depth_attachments[viewport.index()] };
}

void ForwardPlusRenderer::resize_viewport(gfx::Viewport viewport, uint32_t width, uint32_t height) {
    if (width != 0 && height != 0) {
        ctx.resize_attachment(depth_attachments[viewport.index()], { width, height });
    }
}

void ForwardPlusRenderer::create_render_data(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    render_data.camera_ubo = ifc.allocate_scratch_ubo(sizeof(glm::mat4));
    *render_data.camera_ubo.data = scene.cameras[viewport.index()].proj_view;

    render_data.point_lights = ifc.allocate_scratch_ssbo(sizeof(gpu::PointLight) * scene.point_lights.size());
    std::memcpy(render_data.point_lights.data, scene.point_lights.data(), render_data.point_lights.range);

    // We can have at most MAX_LIGHTS in every tile, and there are ceil(W/TILE_SIZE) * ceil(H/TILE_SIZE) tiles, which gives us the size of the buffer

    uint32_t const n_tiles =  static_cast<uint32_t>(std::ceil((float)viewport.width() / ANDROMEDA_TILE_SIZE)) * std::ceil((float)viewport.height() / ANDROMEDA_TILE_SIZE);
    uint32_t const size = sizeof(uint32_t) * n_tiles * ANDROMEDA_MAX_LIGHTS;
    render_data.culled_lights = ifc.allocate_scratch_ssbo(size);
    // Fill with -1 as this indicates an invalid index into the buffer.
    // This way we don't have to manually mark the end of the buffer in the compute shader.
    std::memset(render_data.culled_lights.data, -1, size);
}

ph::Pass ForwardPlusRenderer::depth_prepass(ph::InFlightContext& ifc, gfx::Viewport viewport, gfx::SceneDescription const& scene) {
    ph::Pass depth_pass = ph::PassBuilder::create("fwd_plus_depth")
        .add_depth_attachment(depth_attachments[viewport.index()], ph::LoadOp::Clear, {.depth_stencil = {.depth = 1.0f, .stencil = 0}})
        .execute([this, &ifc, &scene, &viewport](ph::CommandBuffer& cmd) {
            cmd.bind_pipeline("depth_only");
            cmd.auto_viewport_scissor();

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_uniform_buffer("camera", render_data.camera_ubo)
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
    ph::Pass pass = ph::PassBuilder::create_compute("fwd_plus_light_cull")
        .sample_attachment(depth_attachments[viewport.index()], ph::PipelineStage::ComputeShader)
        .execute([this, &ifc, &scene, &viewport](ph::CommandBuffer& cmd) {

        })
        .get();
}

}