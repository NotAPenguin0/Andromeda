#include <andromeda/graphics/backend/atmosphere.hpp>

namespace andromeda::gfx::backend {

AtmosphereRendering::AtmosphereRendering(gfx::Context& ctx, VkSampleCountFlagBits samples, float sample_ratio) : ctx(ctx) {
    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "transmittance")
            .set_shader("data/shaders/transmittance_lut.comp.spv", "main")
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "sky_view")
            .set_shader("data/shaders/sky_view.comp.spv", "main")
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

    {
        // TODO: Remove depth test as we want to add some sort of height fog to visible geometry.
        //       For now we will just render the atmosphere behind everything and ignore this.
        // TODO: move
        // This pipeline will draw a full screen quad at depth == 1.0 to render it behind everything else.
        ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(ctx, "atmosphere")
            .add_shader("data/shaders/atmosphere.vert.spv", "main", ph::ShaderStage::Vertex)
            .add_shader("data/shaders/atmosphere.frag.spv", "main", ph::ShaderStage::Fragment)
            .set_depth_test(true)
            .set_depth_op(VK_COMPARE_OP_LESS_OR_EQUAL)
            .set_depth_write(false)
            .set_cull_mode(VK_CULL_MODE_NONE)
            // Additive blend
            .add_blend_attachment(true, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD)
            .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
            .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
            .set_samples(samples)
            .set_sample_shading(sample_ratio)
            .reflect()
            .get();
        ctx.create_named_pipeline(std::move(pci));
    }

    transmittance = ctx.create_image(ph::ImageType::StorageImage, { ANDROMEDA_TRANSMITTANCE_LUT_WIDTH, ANDROMEDA_TRANSMITTANCE_LUT_HEIGHT }, VK_FORMAT_R32G32B32A32_SFLOAT);
    transmittance_view = ctx.create_image_view(transmittance);
    ctx.name_object(transmittance, "Transmittance LUT");
    ctx.name_object(transmittance_view, "Transmittance LUT - View");

    sky = ctx.create_image(ph::ImageType::StorageImage, { ANDROMEDA_SKY_VIEW_LUT_WIDTH, ANDROMEDA_SKY_VIEW_LUT_HEIGHT }, VK_FORMAT_R32G32B32A32_SFLOAT);
    sky_view = ctx.create_image_view(sky);
    ctx.name_object(sky, "Sky-View LUT");
    ctx.name_object(sky_view, "Sky-View LUT - View");
}

AtmosphereRendering::~AtmosphereRendering() {
    ctx.destroy_image_view(transmittance_view);
    ctx.destroy_image(transmittance);

    ctx.destroy_image_view(sky_view);
    ctx.destroy_image(sky);
}

ph::Pass AtmosphereRendering::lut_update_pass(gfx::Viewport const& vp, ph::InFlightContext& ifc, gfx::SceneDescription const& scene) {
    ph::Pass pass = ph::PassBuilder::create_compute("atmosphere_lut_update")
        .write_storage_image(transmittance_view, ph::PipelineStage::ComputeShader)
        .write_storage_image(sky_view, ph::PipelineStage::ComputeShader)
        .read_storage_image(transmittance_view, ph::PipelineStage::ComputeShader)
        .execute([this, &scene, &vp, &ifc](ph::CommandBuffer& cmd) {
            {
                cmd.bind_compute_pipeline("transmittance");
                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_storage_image("lut", transmittance_view)
                    .get();
                cmd.bind_descriptor_set(set);
                uint32_t dispatches_x = ANDROMEDA_TRANSMITTANCE_LUT_WIDTH / 16;
                uint32_t dispatches_y = ANDROMEDA_TRANSMITTANCE_LUT_HEIGHT / 16;
                vkCmdDispatch_Tracked(cmd, dispatches_x, dispatches_y, 1);

                cmd.bind_compute_pipeline("sky_view");
                set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_storage_image("lut", sky_view)
                    .add_storage_image("transmittance", transmittance_view)
                    .get();
                cmd.bind_descriptor_set(set);

                glm::vec4 pc[3] {
                    glm::vec4(scene.get_camera_info(vp).position, 0.0),
                    scene.get_directional_lights()[0].direction_shadow,
                    scene.get_directional_lights()[0].color_intensity
                };
                cmd.push_constants(ph::ShaderStage::Compute, 0, 3 * sizeof(glm::vec4), &pc);

                // Barrier to protect transmittance image
                VkImageMemoryBarrier barrier {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = nullptr;
                barrier.image = transmittance.handle;
                barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                cmd.barrier(ph::PipelineStage::ComputeShader, ph::PipelineStage::ComputeShader, barrier);

                dispatches_x = ANDROMEDA_SKY_VIEW_LUT_WIDTH / 10;
                dispatches_y = ANDROMEDA_SKY_VIEW_LUT_HEIGHT / 10;
                vkCmdDispatch_Tracked(cmd, dispatches_x, dispatches_y, 1);
            }
        })
        .get();

    return pass;
}

ph::Pass AtmosphereRendering::render_atmosphere_pass(const gfx::Viewport& vp, ph::InFlightContext& ifc, std::string_view output, std::string_view depth, const gfx::SceneDescription& scene) {
    return ph::PassBuilder::create("atmosphere")
        .add_attachment(output, ph::LoadOp::Load)
        .add_depth_attachment(depth, ph::LoadOp::Load)
        .execute([this, &scene, &vp, &ifc](ph::CommandBuffer& cmd) {

        })
        .get();
}

auto AtmosphereRendering::get_luts() const -> LUTs {
    return LUTs {
        .transmittance = transmittance_view,
        .sky_view = sky_view
    };
}

}