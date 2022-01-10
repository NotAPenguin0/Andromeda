#include <andromeda/graphics/backend/debug_geometry.hpp>

namespace andromeda::gfx::backend {

namespace impl {
DebugGeometryList* _debug_geometry_list_ptr = nullptr;
}

void DebugGeometryList::add_line(gfx::Viewport const& viewport, glm::vec3 p1, glm::vec3 p2) {
    auto& vp_data = vp[viewport.index()];
    Command cmd{};
    cmd.type = CommandType::DrawLine;
    cmd.draw.vbo_offset = vp_data.vertices.size();
    cmd.draw.num_vertices = 2; // lines have 2 vertices.
    vp_data.commands.push_back(cmd);
    vp_data.vertices.push_back(p1);
    vp_data.vertices.push_back(p2);
}


void DebugGeometryList::set_color(gfx::Viewport const& viewport, glm::vec3 color) {
    Command cmd{};
    cmd.type = CommandType::PushColor;
    cmd.color = {color};
    vp[viewport.index()].commands.push_back(cmd);
}

void DebugGeometryList::initialize(gfx::Context& ctx) {
    ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "debug_draw_lines")
        .add_shader("data/shaders/debug_draw.vert.spv", "main", ph::ShaderStage::Vertex)
        .add_shader("data/shaders/debug_draw.frag.spv", "main", ph::ShaderStage::Fragment)
        .set_polygon_mode(VK_POLYGON_MODE_LINE)
        .add_blend_attachment(false)
        .set_depth_test(false)
        .set_depth_write(false)
        .add_vertex_input(0)
        .add_vertex_attribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT) // iPos
        .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
        .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
        .reflect()
        .get();
    pci.input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    ctx.create_named_pipeline(std::move(pci));
}

void DebugGeometryList::clear(gfx::Viewport const& viewport) {
    vp[viewport.index()].commands.clear();
    vp[viewport.index()].vertices.clear();
}

ph::Pass DebugGeometryList::build_render_pass(gfx::Viewport const& viewport, gfx::Context& ctx, ph::InFlightContext& ifc, glm::mat4 const& pv) {
    ph::Pass pass = ph::PassBuilder::create("debug_draw")
        .add_attachment(viewport.target(), ph::LoadOp::Load, {})
        .execute([this, &ctx, &ifc, viewport, pv](ph::CommandBuffer& cmd) {
            auto& vp_data = vp[viewport.index()];
            ph::TypedBufferSlice<glm::vec3> vertices = ifc.allocate_scratch_vbo(vp_data.vertices.size() * sizeof(glm::vec3));
            std::memcpy(vertices.data, vp_data.vertices.data(), vertices.range);

            ph::TypedBufferSlice<glm::mat4> cam_data = ifc.allocate_scratch_ubo(sizeof(glm::mat4));
            cam_data.data[0] = pv;

            cmd.bind_pipeline("debug_draw_lines");
            cmd.auto_viewport_scissor();
            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                .add_uniform_buffer("camera", cam_data)
                .get();
            cmd.bind_descriptor_set(set);
            cmd.bind_vertex_buffer(0, vertices);

            for (auto const& command: vp_data.commands) {
                if (command.type == CommandType::PushColor) {
                    glm::vec4 value = glm::vec4(command.color.color, 1.0);
                    cmd.push_constants(ph::ShaderStage::Fragment, 0, sizeof(glm::vec4), &value);
                } else if (command.type == CommandType::DrawLine) {
                    cmd.draw(command.draw.num_vertices, 1, command.draw.vbo_offset, 0);
                }
            }
        })
        .get();

    return pass;
}

}