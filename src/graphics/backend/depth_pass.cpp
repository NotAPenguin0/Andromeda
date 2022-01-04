#include <andromeda/graphics/backend/depth_pass.hpp>
#include <andromeda/graphics/backend/mesh_draw.hpp>

namespace andromeda::gfx::backend {

void create_depth_only_pipeline(gfx::Context& ctx, VkSampleCountFlagBits samples, float sample_ratio) {
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
        .set_samples(samples)
        .set_sample_shading(sample_ratio)
        .reflect()
        .get();
    ctx.create_named_pipeline(std::move(pci));
}

ph::Pass build_depth_pass(gfx::Context& ctx, ph::InFlightContext& ifc, std::string_view target, gfx::SceneDescription const& scene, ph::BufferSlice camera, ph::BufferSlice transforms) {
    ph::Pass pass = ph::PassBuilder::create("fwd_plus_depth")
        .add_depth_attachment(target, ph::LoadOp::Clear, {.depth_stencil = {.depth = 1.0f, .stencil = 0}})
        .execute([&ctx, &ifc, &scene, camera, transforms](ph::CommandBuffer& cmd) {
            cmd.bind_pipeline("depth_only");
            cmd.auto_viewport_scissor();

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_uniform_buffer("camera", camera)
                    .add_storage_buffer("transforms", transforms)
                    .get();
            cmd.bind_descriptor_set(set);

            for_each_ready_mesh(scene, [&cmd](auto const& _, gfx::Mesh const& mesh, uint32_t index) {
                cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(uint32_t), &index); // mesh index is also the transform index
                bind_and_draw(cmd, mesh);
            });
        })
        .get();
    return pass;
}

}