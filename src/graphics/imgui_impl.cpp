#include <andromeda/graphics/imgui.hpp>

#include <andromeda/app/log.hpp>

#include <andromeda/graphics/imgui_impl_glfw.hpp>

namespace andromeda::gfx::imgui {

namespace impl {

static gfx::Context* context = nullptr;
static VkSampler sampler = nullptr;
static ph::RawImage font_image{};
static ph::ImageView font_view{};

static VkSampler create_sampler() {
    VkSamplerCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.pNext = nullptr;
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_LINEAR;
    sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.minLod = 0;
    sci.maxLod = 1;
    sci.maxAnisotropy = 1.0f;
    return context->create_sampler(sci);
}

void create_pipeline() {
    // Create ImGui pipeline
    ph::PipelineCreateInfo pci = ph::PipelineBuilder::create(*context, "imgui_impl_phobos")
        .add_shader("data/shaders/imgui.vert.spv", "main", ph::ShaderStage::Vertex)
        .add_shader("data/shaders/imgui.frag.spv", "main", ph::ShaderStage::Fragment)
        .add_vertex_input(0)
        .add_vertex_attribute(0, 0, VK_FORMAT_R32G32_SFLOAT) // pos
        .add_vertex_attribute(0, 1, VK_FORMAT_R32G32_SFLOAT) // uv
        .add_vertex_attribute(0, 2, VK_FORMAT_R8G8B8A8_UNORM) // col
        .set_cull_mode(VK_CULL_MODE_NONE)
        .enable_blend_op(false)
        .add_blend_attachment(true,
                              VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
                              VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD)
        .add_dynamic_state(VK_DYNAMIC_STATE_SCISSOR)
        .add_dynamic_state(VK_DYNAMIC_STATE_VIEWPORT)
        .set_depth_test(false)
        .set_depth_write(false)
        .reflect()
        .get();

    impl::context->create_named_pipeline(std::move(pci));
}

void fill_buffer_data(ph::TypedBufferSlice<ImDrawVert>& vertex, ph::TypedBufferSlice<ImDrawIdx>& index, ImDrawData* data) {
    ImDrawVert* vtx = vertex.data;
    ImDrawIdx* idx = index.data;
    for (size_t i = 0; i < (size_t) data->CmdListsCount; ++i) {
        ImDrawList const* cmd_list = data->CmdLists[i];
        // Copy data
        memcpy(vtx, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        // Advance pointer
        vtx += cmd_list->VtxBuffer.Size;
        idx += cmd_list->IdxBuffer.Size;
    }

    // Flush the mapped memory range
    context->flush_memory(vertex);
    context->flush_memory(index);
}

void create_fonts_texture() {
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels = nullptr;
    int w = 0, h = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    uint32_t width = w, height = h;
    size_t upload_size = width * height * 4 * sizeof(char);

    impl::font_image = impl::context->create_image(ph::ImageType::Texture, {width, height}, VK_FORMAT_R8G8B8A8_UNORM);
    impl::font_view = impl::context->create_image_view(impl::font_image);

    impl::context->name_object(impl::font_image.handle, "ImGui Font Image");
    impl::context->name_object(impl::font_view, "ImGui Font ImageView");

    // Store our identifier
    io.Fonts->TexID = get_texture_id(impl::font_view);

    ph::RawBuffer staging = impl::context->create_buffer(ph::BufferType::TransferBuffer, upload_size);
    // Upload to buffer
    std::byte* data = impl::context->map_memory(staging);
    std::memcpy(data, pixels, upload_size);

    ph::Queue& transfer = *impl::context->get_queue(ph::QueueType::Transfer);

    VkFence fence = impl::context->create_fence();

    ph::CommandBuffer cmd_buf = transfer.begin_single_time(0);
    cmd_buf.transition_layout(
        ph::PipelineStage::TopOfPipe, VK_ACCESS_NONE_KHR,
        ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
        impl::font_view, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    cmd_buf.copy_buffer_to_image(staging, impl::font_view);
    cmd_buf.transition_layout(
        ph::PipelineStage::Transfer, VK_ACCESS_MEMORY_WRITE_BIT,
        ph::PipelineStage::BottomOfPipe, VK_ACCESS_MEMORY_READ_BIT,
        impl::font_view, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    transfer.end_single_time(cmd_buf, fence);

    impl::context->wait_for_fence(fence);

    impl::context->destroy_buffer(staging);
    impl::context->destroy_fence(fence);
    transfer.free_single_time(cmd_buf, 0);

    LOG_FORMAT(LogLevel::Performance, "Created ImGui font texture ({:.1f} KiB)", upload_size / 1024.0f);
}

void destroy_fonts_texture() {
    impl::context->destroy_image_view(impl::font_view);
    impl::context->destroy_image(impl::font_image);
}

} // namespace impl



void init(gfx::Context& ctx, Window& window) {
    // Save context pointer
    impl::context = &ctx;
    // Register backend with ImGui.
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_phobos";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    io.Fonts->AddFontDefault();

    impl::sampler = impl::create_sampler();
    impl::create_pipeline();
    impl::create_fonts_texture();

    ImGui_ImplGlfw_InitForVulkan(reinterpret_cast<GLFWwindow*>(window.platform_handle()), true);
}

void new_frame() {
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void render(ph::RenderGraph& graph, ph::InFlightContext& ifc, std::string_view target) {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    uint32_t fb_width = (uint32_t) (draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    uint32_t fb_height = (uint32_t) (draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount == 0) {
        return;
    }

    ph::PassBuilder builder = ph::PassBuilder::create("imgui_impl_phobos")
        .add_attachment(target, ph::LoadOp::Load);

    // Check if any of the sampled images are attachments
    for (size_t i = 0; i < (size_t) draw_data->CmdListsCount; ++i) {
        ImDrawList const* draw_list = draw_data->CmdLists[i];
        for (size_t cmd_i = 0; cmd_i < (size_t) draw_list->CmdBuffer.Size; ++cmd_i) {
            ImDrawCmd const* cmd = &draw_list->CmdBuffer[cmd_i];
            ph::ImageView view = impl::context->get_image_view(reinterpret_cast<uint64_t>(cmd->GetTexID()));
            // Font is definitely not an attachment, easy skip
            if (view.id == impl::font_view.id) {
                continue;
            }
            if (impl::context->is_attachment(view)) {
                builder.sample_attachment(impl::context->get_attachment_name(view), ph::PipelineStage::FragmentShader);
            }
        }
    }


    builder.execute([draw_data, &ifc, fb_width, fb_height](ph::CommandBuffer& cmd_buf) {
        cmd_buf.bind_pipeline("imgui_impl_phobos");

        // Write vertex and index buffers
        auto vertex_buffer = ifc.allocate_scratch_vbo<ImDrawVert>(draw_data->TotalVtxCount * sizeof(ImDrawVert));
        auto index_buffer = ifc.allocate_scratch_ibo<ImDrawIdx>(draw_data->TotalIdxCount * sizeof(ImDrawIdx));

        impl::fill_buffer_data(vertex_buffer, index_buffer, draw_data);
        cmd_buf.bind_index_buffer(index_buffer, VK_INDEX_TYPE_UINT16);
        cmd_buf.bind_vertex_buffer(0, vertex_buffer);

        VkIndexType type = VK_INDEX_TYPE_UINT16;
        if (sizeof(ImDrawIdx) == 4) {
            type = VK_INDEX_TYPE_UINT32;
        }

        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float) fb_width;
        viewport.height = (float) fb_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmd_buf.set_viewport(viewport);

        float pc[4]{}; // 0-1: scale, 2-3: translate
        pc[0] = 2.0f / draw_data->DisplaySize.x;
        pc[1] = 2.0f / draw_data->DisplaySize.y;
        pc[2] = -1.0f - draw_data->DisplayPos.x * pc[0];
        pc[3] = -1.0f - draw_data->DisplayPos.y * pc[1];
        cmd_buf.push_constants(ph::ShaderStage::Vertex, 0, 4 * sizeof(float), pc);

        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewportManager
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        size_t global_vtx_offset = 0;
        size_t global_idx_offset = 0;
        for (size_t i = 0; i < (size_t) draw_data->CmdListsCount; ++i) {
            ImDrawList const* cmd_list = draw_data->CmdLists[i];
            for (size_t cmd_i = 0; cmd_i < (size_t) cmd_list->CmdBuffer.Size; ++cmd_i) {
                ImDrawCmd const* cmd = &cmd_list->CmdBuffer[cmd_i];
                // We don't support user callbacks yet
                ImVec4 clip_rect;
                clip_rect.x = (cmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (cmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (cmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (cmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clip_rect.x < 0.0f) {
                        clip_rect.x = 0.0f;
                    }
                    if (clip_rect.y < 0.0f) {
                        clip_rect.y = 0.0f;
                    }

                    VkRect2D scissor;
                    scissor.offset.x = (int32_t) (clip_rect.x);
                    scissor.offset.y = (int32_t) (clip_rect.y);
                    scissor.extent.width = (uint32_t) (clip_rect.z - clip_rect.x);
                    scissor.extent.height = (uint32_t) (clip_rect.w - clip_rect.y);

                    cmd_buf.set_scissor(scissor);
                    // Bind descriptorset with font or user texture
                    ph::ImageView img_view = impl::context->get_image_view(reinterpret_cast<uint64_t>(cmd->GetTexID()));

                    int depth_val = 0;
                    if (img_view.aspect == ph::ImageAspect::Depth) {
                        depth_val = 1;
                    }
                    cmd_buf.push_constants(ph::ShaderStage::Fragment, 4 * sizeof(float), sizeof(uint32_t), &depth_val);

/*                    int user_tex_val = 0;
                    if (img_view != impl::font_view) user_tex_val = 1;
                    cmd_buf.push_constants(ph::ShaderStage::Fragment, 4 * sizeof(float) + sizeof(int), sizeof(int), &user_tex_val);
*/
                    VkDescriptorSet set = ph::DescriptorBuilder::create(*impl::context,
                                                                        cmd_buf.get_bound_pipeline())
                        .add_sampled_image("sTexture", img_view, impl::sampler)
                        .get();

                    cmd_buf.bind_descriptor_set(set);
                    cmd_buf.draw_indexed(cmd->ElemCount, 1, cmd->IdxOffset + global_idx_offset, cmd->VtxOffset + global_vtx_offset, 0);
                }
            }
            global_idx_offset += cmd_list->IdxBuffer.Size;
            global_vtx_offset += cmd_list->VtxBuffer.Size;
        }
    });

    ph::Pass pass = builder.get();
    graph.add_pass(std::move(pass));
}

void shutdown() {
    impl::context->destroy_sampler(impl::sampler);
    impl::destroy_fonts_texture();
}

ImTextureID get_texture_id(ph::ImageView view) {
    return reinterpret_cast<ImTextureID>(view.id);
}

void reload_fonts() {
    impl::destroy_fonts_texture();
    impl::create_fonts_texture();
}

} // namespace andromeda::gfx::imgui