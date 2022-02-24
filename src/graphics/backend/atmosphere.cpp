#include <andromeda/graphics/backend/atmosphere.hpp>

namespace andromeda::gfx::backend {

AtmosphereRendering::AtmosphereRendering(gfx::Context& ctx) : ctx(ctx) {
    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "transmittance")
            .set_shader("data/shaders/transmittance_lut.comp.spv", "main")
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

    transmittance = ctx.create_image(ph::ImageType::StorageImage, { ANDROMEDA_TRANSMITTANCE_LUT_WIDTH, ANDROMEDA_TRANSMITTANCE_LUT_HEIGHT }, VK_FORMAT_R32G32B32A32_SFLOAT);
    transmittance_view = ctx.create_image_view(transmittance);
    ctx.name_object(transmittance, "Transmittance LUT");
    ctx.name_object(transmittance_view, "Transmittance LUT - View");
}

AtmosphereRendering::~AtmosphereRendering() {
    ctx.destroy_image_view(transmittance_view);
    ctx.destroy_image(transmittance);
}

ph::Pass AtmosphereRendering::lut_update_pass(ph::InFlightContext& ifc, gfx::SceneDescription const& scene) {
    ph::Pass pass = ph::PassBuilder::create_compute("atmosphere_lut_update")
        .write_storage_image(transmittance_view, ph::PipelineStage::ComputeShader)
        .execute([this, &ifc](ph::CommandBuffer& cmd) {
            {
                cmd.bind_compute_pipeline("transmittance");
                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_storage_image("lut", transmittance_view)
                    .get();
                cmd.bind_descriptor_set(set);
                uint32_t dispatches_x = ANDROMEDA_TRANSMITTANCE_LUT_WIDTH / 16;
                uint32_t dispatches_y = ANDROMEDA_TRANSMITTANCE_LUT_HEIGHT / 16;
                vkCmdDispatch_Tracked(cmd, dispatches_x, dispatches_y, 1);
            }
        })
        .get();

    return pass;
}

auto AtmosphereRendering::get_luts() const -> LUTs {
    return LUTs {
        .transmittance = transmittance_view
    };
}

}