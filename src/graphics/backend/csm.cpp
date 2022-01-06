#include <andromeda/graphics/backend/csm.hpp>
#include <andromeda/graphics/backend/mesh_draw.hpp>

#include <andromeda/graphics/backend/debug_geometry.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace andromeda::gfx::backend {

bool CascadedShadowMapping::is_shadow_caster(gpu::DirectionalLight const& light) {
    return light.direction_shadow.w >= 0;
}

// Only meaningful if is_shadow_caster(light) == true
uint32_t CascadedShadowMapping::get_light_index(gpu::DirectionalLight const& light) {
    return static_cast<uint32_t>(light.direction_shadow.w);
}

VkSampler CascadedShadowMapping::create_sampler(gfx::Context& ctx) {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.mipLodBias = 0.0f;
    info.maxAnisotropy = 1.0f;
    info.minLod = 0.0f;
    info.maxLod = 1.0f;
    info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    info.compareEnable = true;
    info.compareOp = VK_COMPARE_OP_LESS;
    return ctx.create_sampler(info);
}

void CascadedShadowMapping::free(gfx::Context& ctx) {
    for (auto& vp : viewport_data) {
        for (auto& map : vp.maps) {
            // Skip empty shadow maps
            if (map.attachment.empty()) continue;
            for (auto& cascade : map.cascades) {
                ctx.destroy_image_view(cascade.view);
            }
            ctx.destroy_image_view(map.full_view);
        }
    }
}

void CascadedShadowMapping::create_pipeline(gfx::Context& ctx) {
    ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "shadow")
        .add_shader("data/shaders/shadow.vert.spv", "main", ph::ShaderStage::Vertex)
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
        .set_depth_clamp(true)
        .set_depth_op(VK_COMPARE_OP_LESS_OR_EQUAL)
        .set_cull_mode(VK_CULL_MODE_BACK_BIT)
        .reflect()
        .get();
    ctx.create_named_pipeline(std::move(pci));
}

std::vector<ph::Pass> CascadedShadowMapping::build_shadow_map_passes(gfx::Context& ctx, ph::InFlightContext& ifc, gfx::Viewport const& viewport,
                                                                     gfx::SceneDescription const& scene, ph::BufferSlice transforms) {
    std::vector<ph::Pass> passes;
    passes.reserve(ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS * ANDROMEDA_SHADOW_CASCADE_COUNT);

    for (auto const& dir_light : scene.get_directional_lights()) {
        // Skip lights that are not shadow casters
        if (!is_shadow_caster(dir_light)) continue;

        uint32_t const light_index = get_light_index(dir_light);
        // Request a shadowmap for this light and viewport.
        CascadeMap& shadow_map = request_shadow_map(ctx, viewport, light_index);
        // Calculate projection matrices for all cascades.
        calculate_cascade_splits(shadow_map, viewport, scene.get_camera_info(viewport), dir_light);

        // Write projection matrices to a buffer that we can use in the shader
        ph::TypedBufferSlice<glm::mat4> light_pvs = ifc.allocate_scratch_ubo<glm::mat4>(ANDROMEDA_SHADOW_CASCADE_COUNT * sizeof(glm::mat4));
        for (uint32_t c = 0; c < ANDROMEDA_SHADOW_CASCADE_COUNT; ++c) {
            light_pvs.data[c] = shadow_map.cascades[c].light_proj_view;
        }

        // Start building passes, one for each cascade
        for (uint32_t c = 0; c < ANDROMEDA_SHADOW_CASCADE_COUNT; ++c) {
            std::string pass_name = fmt::vformat("shadow [L{}][C{}]", fmt::make_format_args(light_index, c));
            auto builder = ph::PassBuilder::create(pass_name);

            Cascade& cascade = shadow_map.cascades[c];
            builder.add_depth_attachment(shadow_map.attachment, cascade.view, ph::LoadOp::Clear, { .depth_stencil = { 1.0, 0 } });
            builder.execute([&ctx, &scene, transforms, c, light_pvs](ph::CommandBuffer& cmd) {
                cmd.auto_viewport_scissor();
                cmd.bind_pipeline("shadow");

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_uniform_buffer("light", light_pvs)
                    .add_storage_buffer("transforms", transforms)
                    .get();
                cmd.bind_descriptor_set(set);

                // Push current cascade index to shader
                cmd.push_constants(ph::ShaderStage::Vertex, 0, sizeof(uint32_t), &c);
                // Render scene from light POV.
                for_each_ready_mesh(scene, [&cmd](auto const& draw, auto const& mesh, auto index) {
                    // Skip draws that don't cast shadows
                    if (!draw.occluder) return;
                    // Push transform index to shader and draw
                    cmd.push_constants(ph::ShaderStage::Vertex, sizeof(uint32_t), sizeof(uint32_t), &index);
                    bind_and_draw(cmd, mesh);
                });
            });
            passes.push_back(builder.get());
        }
    }

    return passes;
}

void CascadedShadowMapping::register_attachment_dependencies(gfx::Viewport const& viewport, ph::PassBuilder& builder) {
    auto const& vp = viewport_data[viewport.index()];
    for (CascadeMap const& shadow_map : vp.maps) {
        // Skip unused shadow maps
        if (shadow_map.attachment.empty()) continue;
        // Since the render graph will always synchronize entire images, we can get away with only registering
        // one dependency for each cascade map, for any cascade layer.
        builder.sample_attachment(shadow_map.attachment, shadow_map.cascades[0].view, ph::PipelineStage::FragmentShader);
    }
}

std::vector<std::string_view> CascadedShadowMapping::get_debug_attachments(const gfx::Viewport &viewport) {
    std::vector<std::string_view> result;
    for (auto const& map : viewport_data[viewport.index()].maps) {
        if (map.attachment.empty()) continue;
        result.push_back(map.attachment);
    }
    return result;
}

ph::ImageView CascadedShadowMapping::get_shadow_map(gfx::Viewport const& viewport, gpu::DirectionalLight const& light) {
    CascadeMap& map = viewport_data[viewport.index()].maps[get_light_index(light)];
    assert(!map.attachment.empty() && "No shadow map created for this light");
    return map.full_view;
}

float CascadedShadowMapping::get_split_depth(gfx::Viewport const& viewport, gpu::DirectionalLight const& light, uint32_t const cascade) {
    CascadeMap& map = viewport_data[viewport.index()].maps[get_light_index(light)];
    assert(!map.attachment.empty() && "No shadow map created for this light");
    return map.cascades[cascade].depth_split;
}

glm::mat4 CascadedShadowMapping::get_cascade_matrix(gfx::Viewport const& viewport, gpu::DirectionalLight const& light, uint32_t const cascade) {
    CascadeMap& map = viewport_data[viewport.index()].maps[get_light_index(light)];
    assert(!map.attachment.empty() && "No shadow map created for this light");
    return map.cascades[cascade].light_proj_view;
}

auto CascadedShadowMapping::request_shadow_map(gfx::Context& ctx, gfx::Viewport const& viewport, uint32_t light_index) -> CascadeMap& {
    assert(light_index < ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS && "Light index out of range.");
    ViewportShadowMap& vp = viewport_data[viewport.index()];
    CascadeMap& map = vp.maps[light_index];
    // If the attachment string is not empty this map was created before, and we can simply return it
    if (!map.attachment.empty()) {
        return map;
    }

    // Create attachment and ImageViews for cascades.
    map.attachment = gfx::Viewport::local_string(viewport, fmt::vformat("Cascaded Shadow Map [L{}]", fmt::make_format_args(light_index)));
    ctx.create_attachment(map.attachment, { ANDROMEDA_SHADOW_RESOLUTION, ANDROMEDA_SHADOW_RESOLUTION },
                          VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, ANDROMEDA_SHADOW_CASCADE_COUNT, ph::ImageType::DepthStencilAttachment);
    // Now create an ImageView for each of the cascades individually
    ph::Attachment attachment = ctx.get_attachment(map.attachment);
    for (uint32_t i = 0; i < ANDROMEDA_SHADOW_CASCADE_COUNT; ++i) {
        Cascade& cascade = map.cascades[i];
        // Create image view to mip level 0 of the i-th layer of this image.
        cascade.view = ctx.create_image_view(*attachment.image, 0, i, ph::ImageAspect::Depth);
        ctx.name_object(cascade.view, fmt::vformat("CSM Cascade [L{}][C{}]", fmt::make_format_args(light_index, i)));
    }
    map.full_view = ctx.create_image_view(*attachment.image, ph::ImageAspect::Depth);
    ctx.name_object(map.full_view, fmt::vformat("CSM Cascade [L{}][Full]", fmt::make_format_args(light_index)));
    return map;
}

namespace {

constexpr glm::vec4 base_frustum_corners[8] {
    glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f),
    glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f),
    glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f),
    glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
    glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f),
    glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f),
    glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
    glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
};

}

void CascadedShadowMapping::calculate_cascade_splits(CascadeMap& shadow_map, gfx::Viewport const& viewport, gfx::SceneDescription::CameraInfo const& camera, gpu::DirectionalLight const& light) {
    std::array<float, ANDROMEDA_SHADOW_CASCADE_COUNT> splits {};
    float const near = camera.near;
    float const far = camera.far;
    float const clip_range = far - near;

    float const z_min = near;
    float const z_max = near + clip_range;

    float const z_range = z_max - z_min;
    float const z_ratio = z_max / z_min;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t c = 0; c < ANDROMEDA_SHADOW_CASCADE_COUNT; c++) {
        float const p = (c + 1) / static_cast<float>(ANDROMEDA_SHADOW_CASCADE_COUNT);
        float const log = z_min * std::pow(z_ratio, p);
        float const uniform = z_min + z_range * p;
        float const d = cascade_split_lambda * (log - uniform) + uniform;
        splits[c] = (d - near) / clip_range;
    }

    // Project frustum to world space
    glm::mat4 cam_inverse = glm::inverse(camera.proj_view);
//    cam_inverse[1][1] *= -1;

    // Compute orthographic projection matrix for each cascade
    float last_split = 0.0f;

    for (uint32_t c = 0; c < ANDROMEDA_SHADOW_CASCADE_COUNT; ++c) {
        float const split = splits[c];

        // Projected frustum (Camera -> Worldspace)
        std::array<glm::vec3, 8> frustum{};

        for (uint32_t f = 0; f < frustum.size(); ++f) {
            glm::vec4 const corner_inverse = cam_inverse * base_frustum_corners[f];
            frustum[f] = corner_inverse / corner_inverse.w;
        }

        // Move frustum to match cascade split
        for (uint32_t f = 0; f < frustum.size() / 2; ++f) {
            glm::vec3 const distance = frustum[f + 4] - frustum[f];
            // Move the far corner to the new split distance
            frustum[f + 4] = frustum[f] + (distance * split);
            // Move the close corner to the old split distance
            frustum[f] = frustum[f] + (distance * last_split);
        }

        // Compute center of frustum by averaging together all points
        glm::vec3 frustum_center = glm::vec3(0.0f);
        for (auto const& corner : frustum) {
            frustum_center += corner;
            frustum_center /= static_cast<float>(frustum.size());
        }

        // Compute radius of bounding sphere. We do this by finding the largest distance
        // between the center and each of the points.
        float radius = 0.0f;
        for (auto const& corner : frustum) {
            float const distance = glm::length(corner - frustum_center);
            radius = std::max(radius, distance);
        }

        // Round radius
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 const max_extents = glm::vec3(radius);
        glm::vec3 const min_extents = -max_extents;

        glm::vec3 const light_dir = glm::vec3(light.direction_shadow.x, light.direction_shadow.y, light.direction_shadow.z);

        glm::mat4 light_view = glm::lookAt(frustum_center - light_dir * -min_extents.z, frustum_center, glm::vec3(0, 1, 0));
        glm::mat4 light_proj = glm::ortho(min_extents.x, max_extents.x, min_extents.y, max_extents.y, 0.0f, max_extents.z - min_extents.z);

        // Store split distance and matrix in cascade structure
        shadow_map.cascades[c].depth_split = (near + split * clip_range) * -1.0f;
        shadow_map.cascades[c].light_proj_view = light_proj * light_view;

        // Store last split value, so we can correctly build the frustum for next iteration
        last_split = split;
    }
}

}