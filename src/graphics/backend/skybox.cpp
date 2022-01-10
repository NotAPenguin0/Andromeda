#include <andromeda/graphics/backend/skybox.hpp>

namespace andromeda::gfx::backend {

// Vertex data for skybox
namespace {
constexpr float skybox_vertices[] = {
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f
};
}

void create_skybox_pipeline(gfx::Context& ctx, VkSampleCountFlagBits samples, float sample_ratio) {
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
        .set_samples(samples)
        .set_sample_shading(sample_ratio)
        .reflect()
        .get();
    ctx.create_named_pipeline(std::move(pci));
}

void render_skybox(gfx::Context& ctx, ph::InFlightContext& ifc, ph::CommandBuffer& cmd, gfx::Environment const& env, SceneDescription::CameraInfo const& camera) {
    cmd.bind_pipeline("skybox");
    cmd.auto_viewport_scissor();
    ph::BufferSlice vbo = ifc.allocate_scratch_vbo(sizeof(skybox_vertices));
    std::memcpy(vbo.data, skybox_vertices, sizeof(skybox_vertices));
    cmd.bind_vertex_buffer(0, vbo);

    VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
        .add_sampled_image("skybox", env.cubemap_view, ctx.basic_sampler())
        .get();
    cmd.bind_descriptor_set(set);

    cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(glm::mat4), &camera.projection);
    glm::mat4 view_no_position = glm::mat4(glm::mat3(camera.view));
    cmd.push_constants(ph::ShaderStage::Vertex, sizeof(glm::mat4), sizeof(glm::mat4), &view_no_position);
    vkCmdDraw_Tracked(cmd, 36, 1, 0, 0);
}

}