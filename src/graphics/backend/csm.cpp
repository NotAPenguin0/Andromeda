#include <andromeda/graphics/backend/csm.hpp>

namespace andromeda::gfx::backend {

static bool is_shadow_caster(gpu::DirectionalLight const& light) {
    return light.direction_shadow.w >= 0;
}

// Only meaningful if is_shadow_caster(light) == true
static uint32_t get_light_index(gpu::DirectionalLight const& light) {
    return static_cast<uint32_t>(light.direction_shadow.w);
}

void CascadedShadowMapping::create_pipeline(gfx::Context& ctx) {

}

std::vector<ph::Pass> CascadedShadowMapping::build_shadow_map_passes(gfx::Context& ctx, ph::InFlightContext& ifc, gfx::Viewport const& viewport, gfx::SceneDescription const& scene) {
    std::vector<ph::Pass> passes;
    passes.reserve(ANDROMEDA_MAX_SHADOWING_DIRECTIONAL_LIGHTS * ANDROMEDA_SHADOW_CASCADE_COUNT);

    for (auto const& dir_light : scene.get_directional_lights()) {
        // Skip lights that are not shadow casters
        if (!is_shadow_caster(dir_light)) continue;

        uint32_t const light_index = get_light_index(dir_light);
        // Request a shadowmap for this light and viewport.
        CascadeMap& shadow_map = request_shadow_map(ctx, viewport, light_index);

        // Start building passes, one for each cascade
        for (uint32_t c = 0; c < ANDROMEDA_SHADOW_CASCADE_COUNT; ++c) {
            std::string pass_name = fmt::vformat("shadow [L{}][C{}]", fmt::make_format_args(light_index, c));
            auto builder = ph::PassBuilder::create(pass_name);

            Cascade& cascade = shadow_map.cascades[c];
            builder.add_depth_attachment(shadow_map.attachment, cascade.view, ph::LoadOp::Clear, { .depth_stencil = { 1.0, 0 } });
            builder.execute([](ph::CommandBuffer& cmd) {
                cmd.auto_viewport_scissor();
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
    return map;
}

}