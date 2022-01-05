#include <andromeda/graphics/backend/tonemap.hpp>

namespace andromeda::gfx::backend {

void create_luminance_histogram_pipelines(gfx::Context& ctx) {
    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "luminance_accumulate")
                .set_shader("data/shaders/luminance_accumulate.comp.spv", "main")
                .reflect()
                .get();
        ctx.create_named_pipeline(std::move(pci));
    }
    {
        ph::ComputePipelineCreateInfo pci = ph::ComputePipelineBuilder::create(ctx, "luminance_average")
                .set_shader("data/shaders/luminance_average.comp.spv", "main")
                .reflect()
                .get();
        ctx.create_named_pipeline(std::move(pci));
    }
}

void create_tonemapping_pipeline(gfx::Context& ctx) {
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

ph::Pass build_average_luminance_pass(gfx::Context& ctx, ph::InFlightContext& ifc, std::string_view main_hdr, gfx::Viewport const& viewport,
                                      gfx::SceneDescription const& scene, ph::BufferSlice average) {
    ph::BufferSlice histogram = ifc.allocate_scratch_ssbo(ANDROMEDA_LUMINANCE_BINS * sizeof(uint32_t));
    ph::Pass pass = ph::PassBuilder::create_compute("luminance_histogram")
        .sample_attachment(main_hdr, ph::PipelineStage::ComputeShader)
        .shader_write_buffer(histogram, ph::PipelineStage::ComputeShader)
        .shader_read_buffer(histogram, ph::PipelineStage::ComputeShader)
        .shader_write_buffer(average, ph::PipelineStage::ComputeShader)
        .shader_read_buffer(average, ph::PipelineStage::ComputeShader)
        .execute([&ctx, &scene, viewport, main_hdr, average, histogram](ph::CommandBuffer& cmd) {
            auto const& camera = scene.get_camera_info(viewport);
            {
                cmd.bind_compute_pipeline("luminance_accumulate");

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_sampled_image("scene_hdr", ctx.get_attachment(main_hdr).view, ctx.basic_sampler())
                    .add_storage_buffer("histogram", histogram)
                    .get();
                cmd.bind_descriptor_set(set);

                float const pc[2] {
                        camera.min_log_luminance,
                        1.0f / (camera.max_log_luminance - camera.min_log_luminance)
                };
                cmd.push_constants(ph::ShaderStage::Compute, 0, sizeof(pc), &pc);

                uint32_t const dispatches_x = std::ceil(
                        viewport.width() / (float) ANDROMEDA_LUMINANCE_ACCUMULATE_GROUP_SIZE);
                uint32_t const dispatches_y = std::ceil(
                        viewport.height() / (float) ANDROMEDA_LUMINANCE_ACCUMULATE_GROUP_SIZE);
                cmd.dispatch(dispatches_x, dispatches_y, 1);
            }

            // Add a barrier to protect the histogram buffer
            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = histogram.buffer;
            barrier.offset = histogram.offset;
            barrier.size = histogram.range;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            cmd.barrier(ph::PipelineStage::ComputeShader, ph::PipelineStage::ComputeShader, barrier);

            {
                cmd.bind_compute_pipeline("luminance_average");

                VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_storage_buffer("average_luminance", average)
                    .add_storage_buffer("histogram", histogram)
                    .get();
                cmd.bind_descriptor_set(set);

                float const pc[4] {
                        camera.min_log_luminance,
                        camera.max_log_luminance - camera.min_log_luminance,
                        ctx.delta_time(),
                        0.0 // padding
                };

                uint32_t const num_pixels = viewport.width() * viewport.height();

                cmd.push_constants(ph::ShaderStage::Compute, 0, 4 * sizeof(float), &pc);
                cmd.push_constants(ph::ShaderStage::Compute, 4 * sizeof(float), sizeof(uint32_t), &num_pixels);

                cmd.dispatch(1, 1, 1);
            }
        })
        .get();
    return pass;
}

ph::Pass build_tonemap_pass(gfx::Context& ctx, std::string_view main_hdr, std::string_view target, VkSampleCountFlagBits samples, ph::BufferSlice luminance) {
    ph::Pass pass = ph::PassBuilder::create("forward_plus_tonemap")
        .add_attachment(target, ph::LoadOp::Clear, ph::ClearValue{.color = {0.0, 0.0, 0.0, 1.0}})
        .sample_attachment(main_hdr, ph::PipelineStage::FragmentShader)
        .shader_read_buffer(luminance, ph::PipelineStage::FragmentShader)
        .execute([&ctx, main_hdr, samples, luminance](ph::CommandBuffer& cmd) {
            cmd.bind_pipeline("tonemap");
            cmd.auto_viewport_scissor();

            VkDescriptorSet set = ph::DescriptorBuilder::create(ctx, cmd.get_bound_pipeline())
                    .add_sampled_image("input_hdr", ctx.get_attachment(main_hdr).view, ctx.basic_sampler())
                    .add_storage_buffer("average_luminance", luminance)
                    .get();
            cmd.bind_descriptor_set(set);

            cmd.push_constants(ph::ShaderStage::Fragment, 0, sizeof(uint32_t), &samples);
            cmd.draw(6, 1, 0, 0);
        })
    .get();
    return pass;
}

}