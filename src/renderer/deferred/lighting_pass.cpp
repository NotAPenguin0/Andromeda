#include <andromeda/renderer/deferred/lighting_pass.hpp>

#include <andromeda/core/context.hpp>
#include <andromeda/renderer/util.hpp>

#include <phobos/core/vulkan_context.hpp>

#include <stl/literals.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <andromeda/assets/assets.hpp>
#include <phobos/present/present_manager.hpp>

namespace andromeda::renderer::deferred {

LightingPass::LightingPass(Context& ctx, ph::PresentManager& vk_present) {
    create_pipeline(ctx);
    create_ambient_pipeline(ctx);
    create_light_mesh(ctx);

    brdf_lookup = ctx.request_texture("data/textures/ibl_brdf_lut.png", false);
    // TODO: look into vkCmdResolveImage instead
    depth_resolved = &vk_present.add_depth_attachment("resolved_depth", { 1280, 720 });
}

static constexpr float quad_geometry[] = {
    -1, 1, 0, 1, -1, -1, 0, 0,
    1, -1, 1, 0, -1, 1, 0, 1,
    1, -1, 1, 0, 1, 1, 1, 1
};

// Exported from blender
static constexpr float light_volume_geometry[] = {
    0.000000, -1.000000, 0.000000,
    0.723607, -0.447220, 0.525725,
    -0.276388, -0.447220, 0.85064,
    -0.894426, -0.447216, 0.00000,
    -0.276388, -0.447220, -0.8506,
    0.723607, -0.447220, -0.52572,
    0.276388, 0.447220, 0.850649,
    -0.723607, 0.447220, 0.525725,
    -0.723607, 0.447220, -0.52572,
    0.276388, 0.447220, -0.850649,
    0.894426, 0.447216, 0.000000,
    0.000000, 1.000000, 0.000000,
    -0.162456, -0.850654, 0.49999,
    0.425323, -0.850654, 0.309011,
    0.262869, -0.525738, 0.809012,
    0.850648, -0.525736, 0.000000,
    0.425323, -0.850654, -0.30901,
    -0.525730, -0.850652, 0.00000,
    -0.688189, -0.525736, 0.49999,
    -0.162456, -0.850654, -0.4999,
    -0.688189, -0.525736, -0.4999,
    0.262869, -0.525738, -0.80901,
    0.951058, 0.000000, 0.309013,
    0.951058, 0.000000, -0.309013,
    0.000000, 0.000000, 1.000000,
    0.587786, 0.000000, 0.809017,
    -0.951058, 0.000000, 0.309013,
    -0.587786, 0.000000, 0.809017,
    -0.587786, 0.000000, -0.80901,
    -0.951058, 0.000000, -0.30901,
    0.587786, 0.000000, -0.809017,
    0.000000, 0.000000, -1.000000,
    0.688189, 0.525736, 0.499997,
    -0.262869, 0.525738, 0.809012,
    -0.850648, 0.525736, 0.000000,
    -0.262869, 0.525738, -0.80901,
    0.688189, 0.525736, -0.499997,
    0.162456, 0.850654, 0.499995,
    0.525730, 0.850652, 0.000000,
    -0.425323, 0.850654, 0.309011,
    -0.425323, 0.850654, -0.30901,
    0.162456, 0.850654, -0.499995
};
static constexpr uint32_t light_volume_indices[] = {
    0 , 13 , 12 , 1 , 13 , 15 , 0 , 12 , 17 , 0 , 17 , 19 , 0 , 19 , 16 , 1 ,
 15 , 22 , 2 , 14 , 24 , 3 , 18 , 26 , 4 , 20 , 28 , 5 , 21 , 30 , 1 , 22
 , 25 , 2 , 24 , 27 , 3 , 26 , 29 , 4 , 28 , 31 , 5 , 30 , 23 , 6 , 32 ,
37 , 7 , 33 , 39 , 8 , 34 , 40 , 9 , 35 , 41 , 10 , 36 , 38 , 38 , 41 , 11 ,
38 , 36 , 41 , 36 , 9 , 41 , 41 , 40 , 11 , 41 , 35 , 40 , 35 , 8 , 40 ,
40 , 39 , 11 , 40 , 34 , 39 , 34 , 7 , 39 , 39 , 37 , 11 , 39 , 33 ,
37 , 33 , 6 , 37 , 37 , 38 , 11 , 37 , 32 , 38 , 32 , 10 , 38 , 23 , 36 ,
 10 , 23 , 30 , 36 , 30 , 9 , 36 , 31 , 35 , 9 , 31 , 28 , 35 , 28 , 8 ,
35 , 29 , 34 , 8 , 29 , 26 , 34 , 26 , 7 , 34 , 27 , 33 , 7 , 27 , 24 , 33 ,
24 , 6 , 33 , 25 , 32 , 6 , 25 , 22 , 32 , 22 , 10 , 32 , 30 , 31 , 9
 , 30 , 21 , 31 , 21 , 4 , 31 , 28 , 29 , 8 , 28 , 20 , 29 , 20 , 3 , 29
, 26 , 27 , 7 , 26 , 18 , 27 , 18 , 2 , 27 , 24 , 25 , 6 , 24 , 14 , 25 ,
 14 , 1 , 25 , 22 , 23 , 10 , 22 , 15 , 23 , 15 , 5 , 23 , 16 , 21 , 5 ,
16 , 19 , 21 , 19 , 4 , 21 , 19 , 20 , 4 , 19 , 17 , 20 , 17 , 3 , 20 , 17 ,
18 , 3 , 17 , 12 , 18 , 12 , 2 , 18 , 15 , 16 , 5 , 15 , 13 , 16 , 13
 , 0 , 16 , 12 , 14 , 2 , 12 , 13 , 14 , 13 , 1 , 14 ,
};

void LightingPass::build(Context& ctx, Attachments attachments, ph::FrameInfo& frame, ph::RenderGraph& graph, RenderDatabase& database) {
    per_frame_buffers = {};

    ph::RenderPass pass;
#if ANDROMEDA_DEBUG
    pass.debug_name = "deferred_lighting_pass";
#endif
    pass.outputs = { attachments.output, *depth_resolved };
    pass.sampled_attachments = { attachments.normal, attachments.depth, attachments.albedo_ao, attachments.metallic_roughness };
    pass.clear_values = {
        vk::ClearColorValue{ std::array<float, 4>{ {0.0f, 0.0f, 0.0f, 1.0f}} }, 
        vk::ClearDepthStencilValue{1, 0} 
    };

    pass.callback = [this, &ctx, &frame, &database, attachments] (ph::CommandBuffer& cmd_buf) {
        auto_viewport_scissor(cmd_buf);

        // If there are no lights, skip all other work
        if (database.point_lights.empty()) { return; }

        // Lighting pass
        {
            ph::Pipeline pipeline = cmd_buf.get_pipeline("deferred_lighting");
            cmd_buf.bind_pipeline(pipeline);

            update_camera_data(cmd_buf, database);
            update_lights(cmd_buf, database);

            vk::DescriptorSet descr_set = get_descriptors(cmd_buf, frame, attachments);
            cmd_buf.bind_descriptor_set(0, descr_set);

            Mesh* light_mesh = assets::get(light_mesh_handle);

            cmd_buf.bind_vertex_buffer(0, ph::whole_buffer_slice(*ctx.vulkan, light_mesh->get_vertices()));
            cmd_buf.bind_index_buffer(ph::whole_buffer_slice(*ctx.vulkan, light_mesh->get_indices()));

            uint32_t screen_size[] = { attachments.output.get_width(), attachments.output.get_height() };
            cmd_buf.push_constants(vk::ShaderStageFlagBits::eFragment, 0, 2 * sizeof(uint32_t), screen_size);
            cmd_buf.draw_indexed(light_mesh->index_count(), database.point_lights.size(), 0, 0, 0);
        }

        // Ambient lighting pass
        if (enable_ambient && assets::is_ready(database.environment_map) 
            && assets::is_ready(brdf_lookup)) {
            ph::Pipeline pipeline = cmd_buf.get_pipeline("deferred_ambient");
            cmd_buf.bind_pipeline(pipeline);

            ph::DescriptorSetBinding set_binding;
            set_binding.add(ph::make_descriptor(bindings.ambient_albedo_ao, attachments.albedo_ao.image_view(), frame.default_sampler));
            set_binding.add(ph::make_descriptor(bindings.ambient_depth, attachments.depth.image_view(), frame.default_sampler));
            set_binding.add(ph::make_descriptor(bindings.ambient_metallic_roughness, attachments.metallic_roughness.image_view(), frame.default_sampler));
            set_binding.add(ph::make_descriptor(bindings.ambient_normal, attachments.normal.image_view(), frame.default_sampler));
            EnvMap* envmap = assets::get(database.environment_map);
            set_binding.add(ph::make_descriptor(bindings.ambient_irradiance_map, envmap->irradiance_map_view, frame.default_sampler));
            set_binding.add(ph::make_descriptor(bindings.ambient_camera, per_frame_buffers.camera));
            set_binding.add(ph::make_descriptor(bindings.ambient_specular_map, envmap->specular_map_view, frame.default_sampler));
            Texture* lookup = assets::get(brdf_lookup);
            set_binding.add(ph::make_descriptor(bindings.ambient_brdf_lookup, lookup->view, frame.default_sampler));
            vk::DescriptorSet descr_set = cmd_buf.get_descriptor(set_binding);
            cmd_buf.bind_descriptor_set(0, descr_set);

            ph::BufferSlice quad = cmd_buf.allocate_scratch_vbo(sizeof(quad_geometry));
            std::memcpy(quad.data, quad_geometry, sizeof(quad_geometry));
            cmd_buf.bind_vertex_buffer(0, quad);
            cmd_buf.draw(6, 1, 0, 0);
        }
    };

    graph.add_pass(std::move(pass));
}

void LightingPass::create_pipeline(Context& ctx) {
    using namespace stl::literals;

    ph::PipelineCreateInfo pci;

    // We need to blend the resulting light values together
    vk::PipelineColorBlendAttachmentState blend;
    blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    blend.blendEnable = true;
    // Additive blending
    blend.colorBlendOp = vk::BlendOp::eAdd;
    blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    blend.dstColorBlendFactor = vk::BlendFactor::eOne;
    blend.alphaBlendOp = vk::BlendOp::eAdd;
    blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    blend.dstAlphaBlendFactor = vk::BlendFactor::eOne;

    pci.blend_attachments.push_back(blend);
#if ANDROMEDA_DEBUG
    pci.debug_name = "deferred_lighting_pipeline";
#endif

    // To avoid having to recreate the pipeline when the viewport is changed
    pci.dynamic_states.push_back(vk::DynamicState::eViewport);
    pci.dynamic_states.push_back(vk::DynamicState::eScissor);
    // Even though they are dynamic states, viewportCount must be 1 if the multiple viewports feature is  not enabled. The pViewports field
    // is ignored though, so the actual values don't matter. 
    // See  also https://renderdoc.org/vkspec_chunked/chap25.html#VkPipelineViewportStateCreateInfo
    pci.viewports.emplace_back();
    pci.scissors.emplace_back();

    // The lighting pass will render light geometry. We don't need to shade this light geometry, so only a vec3 for the position is enough.
    // Note that we will calculate screenspace texture coordinates to sample the Gbuffer in the vertex shader.
    constexpr size_t stride = 3 * sizeof(float);
    pci.vertex_input_bindings.push_back(vk::VertexInputBindingDescription(0, stride, vk::VertexInputRate::eVertex));
    pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32B32Sfloat, 0_u32);

    pci.depth_stencil.depthTestEnable = false;
    pci.depth_stencil.depthWriteEnable = false;
    // If we do not cull any geometry in the lighting pass, all lights will have double the intensity (since front and back faces are both 
    // rendered). The problem with culling backfaces arrives when the camera is inside a light volume: That light volume would no longer be
    // shaded since all back faces are culled. To fix this, we have to cull front faces instead.
    pci.rasterizer.cullMode = vk::CullModeFlagBits::eFront;

    std::vector<uint32_t> vert_code = ph::load_shader_code("data/shaders/lighting.vert.spv");
    std::vector<uint32_t> frag_code = ph::load_shader_code("data/shaders/lighting.frag.spv");

    ph::ShaderHandle vertex_shader = ph::create_shader(*ctx.vulkan, vert_code, "main", vk::ShaderStageFlagBits::eVertex);
    ph::ShaderHandle fragment_shader = ph::create_shader(*ctx.vulkan, frag_code, "main", vk::ShaderStageFlagBits::eFragment);

    pci.shaders.push_back(vertex_shader);
    pci.shaders.push_back(fragment_shader);

    ph::reflect_shaders(*ctx.vulkan, pci);
    // Store bindings so we don't need to look them up every frame
    bindings.depth = pci.shader_info["gDepth"];
    bindings.normal = pci.shader_info["gNormal"];
    bindings.albedo_ao = pci.shader_info["gAlbedoAO"];
    bindings.metallic_roughness = pci.shader_info["gMetallicRoughness"];
    bindings.lights = pci.shader_info["lights"];
    bindings.camera = pci.shader_info["camera"];

    ctx.vulkan->pipelines.create_named_pipeline("deferred_lighting", std::move(pci));
}

void LightingPass::create_ambient_pipeline(Context& ctx) {
    using namespace stl::literals;

    ph::PipelineCreateInfo pci;
    // To avoid having to recreate the pipeline when the viewport is changed
    pci.dynamic_states.push_back(vk::DynamicState::eViewport);
    pci.dynamic_states.push_back(vk::DynamicState::eScissor);
    // Even though they are dynamic states, viewportCount must be 1 if the multiple viewports feature is  not enabled. The pViewports field
    // is ignored though, so the actual values don't matter. 
    // See  also https://renderdoc.org/vkspec_chunked/chap25.html#VkPipelineViewportStateCreateInfo
    pci.viewports.emplace_back();
    pci.scissors.emplace_back();

    // We need to blend the resulting light values together
    vk::PipelineColorBlendAttachmentState blend;
    blend.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    blend.blendEnable = true;
    // Additive blending
    blend.colorBlendOp = vk::BlendOp::eAdd;
    blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    blend.dstColorBlendFactor = vk::BlendFactor::eOne;
    blend.alphaBlendOp = vk::BlendOp::eAdd;
    blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    blend.dstAlphaBlendFactor = vk::BlendFactor::eOne;
    pci.blend_attachments.push_back(blend);
#if ANDROMEDA_DEBUG
    pci.debug_name = "deferred_ambient_pipeline";
#endif
    constexpr size_t stride = 4 * sizeof(float);
    pci.vertex_input_bindings.push_back(vk::VertexInputBindingDescription(0, stride, vk::VertexInputRate::eVertex));
    pci.vertex_attributes.emplace_back(0_u32, 0_u32, vk::Format::eR32G32Sfloat, 0_u32);
    pci.vertex_attributes.emplace_back(1_u32, 0_u32, vk::Format::eR32G32Sfloat, 2 * (uint32_t)sizeof(float));

    pci.depth_stencil.depthTestEnable = false;
    pci.depth_stencil.depthWriteEnable = true;
    pci.depth_stencil.depthBoundsTestEnable = false;
    // The ambient pipeline draws a fullscreen quad so we don't want any culling
    pci.rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    std::vector<uint32_t> vert_code = ph::load_shader_code("data/shaders/ambient.vert.spv");
    std::vector<uint32_t> frag_code = ph::load_shader_code("data/shaders/ambient.frag.spv");

    ph::ShaderHandle vertex_shader = ph::create_shader(*ctx.vulkan, vert_code, "main", vk::ShaderStageFlagBits::eVertex);
    ph::ShaderHandle fragment_shader = ph::create_shader(*ctx.vulkan, frag_code, "main", vk::ShaderStageFlagBits::eFragment);

    pci.shaders.push_back(vertex_shader);
    pci.shaders.push_back(fragment_shader);

    ph::reflect_shaders(*ctx.vulkan, pci);
    // Store bindings so we don't need to look them up every frame
    bindings.ambient_albedo_ao = pci.shader_info["gAlbedoAO"];
    bindings.ambient_depth = pci.shader_info["gDepth"];
    bindings.ambient_normal = pci.shader_info["gNormal"];
    bindings.ambient_metallic_roughness = pci.shader_info["gMetallicRoughness"];
    bindings.ambient_camera = pci.shader_info["camera"];
    bindings.ambient_irradiance_map = pci.shader_info["irradiance_map"];
    bindings.ambient_brdf_lookup = pci.shader_info["brdf_lookup"];
    bindings.ambient_specular_map = pci.shader_info["specular_map"];

    ctx.vulkan->pipelines.create_named_pipeline("deferred_ambient", std::move(pci));
}

void LightingPass::create_light_mesh(Context& ctx) {
    light_mesh_handle = ctx.request_mesh(
        light_volume_geometry, sizeof(light_volume_geometry) / sizeof(float), 
        light_volume_indices, sizeof(light_volume_indices) / sizeof(uint32_t));
}

void LightingPass::update_camera_data(ph::CommandBuffer& cmd_buf, RenderDatabase& database) {
    vk::DeviceSize const size = 3 * sizeof(glm::mat4) + sizeof(glm::vec4);
    per_frame_buffers.camera = cmd_buf.allocate_scratch_ubo(size);
    glm::mat4 inverse_projection = glm::inverse(database.projection);
    glm::mat4 inverse_view = glm::inverse(database.view);
    std::memcpy(per_frame_buffers.camera.data, glm::value_ptr(database.projection_view), sizeof(glm::mat4));
    std::memcpy(per_frame_buffers.camera.data + sizeof(glm::mat4), glm::value_ptr(inverse_projection), sizeof(glm::mat4));
    std::memcpy(per_frame_buffers.camera.data + 2 * sizeof(glm::mat4), glm::value_ptr(inverse_view), sizeof(glm::mat4));
    std::memcpy(per_frame_buffers.camera.data + 3 * sizeof(glm::mat4), glm::value_ptr(database.camera_position), sizeof(glm::vec3));
}

void LightingPass::update_lights(ph::CommandBuffer& cmd_buf, RenderDatabase& database) {
    vk::DeviceSize const size = sizeof(RenderDatabase::InternalPointLight) * database.point_lights.size();
    per_frame_buffers.lights = cmd_buf.allocate_scratch_ssbo(size);
    std::memcpy(per_frame_buffers.lights.data, database.point_lights.data(), size);
}

vk::DescriptorSet LightingPass::get_descriptors(ph::CommandBuffer& cmd_buf, ph::FrameInfo& frame, Attachments attachments) {
    ph::DescriptorSetBinding set;
    set.add(ph::make_descriptor(bindings.camera, per_frame_buffers.camera));
    set.add(ph::make_descriptor(bindings.lights, per_frame_buffers.lights));
    set.add(ph::make_descriptor(bindings.depth, attachments.depth.image_view(), frame.default_sampler));
    set.add(ph::make_descriptor(bindings.albedo_ao, attachments.albedo_ao.image_view(), frame.default_sampler));
    set.add(ph::make_descriptor(bindings.normal, attachments.normal.image_view(), frame.default_sampler));
    set.add(ph::make_descriptor(bindings.metallic_roughness, attachments.metallic_roughness.image_view(), frame.default_sampler));
    return cmd_buf.get_descriptor(set);
}

}