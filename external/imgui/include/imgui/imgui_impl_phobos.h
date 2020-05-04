#ifndef IMGUI_IMPL_PHOBOS_H_
#define IMGUI_IMPL_PHOBOS_H_

#include <imgui/imgui.h>
#include <phobos/forward.hpp>

// meh => we can probably get rid of this somehow
#include <vulkan/vulkan.hpp>

struct ImGui_ImplPhobos_InitInfo {
    ph::VulkanContext* context;
};

void ImGui_ImplPhobos_Init(ImGui_ImplPhobos_InitInfo* init_info);
void ImGui_ImplPhobos_RenderDrawData(ImDrawData* draw_data, ph::FrameInfo* frame, ph::RenderGraph* render_graph, ph::Renderer* renderer);
void ImGui_ImplPhobos_Shutdown();

bool ImGui_ImplPhobos_CreateFontsTexture(vk::CommandBuffer cmd_buf);
void ImGui_ImplPhobos_DestroyFontUploadObjects();

void* ImGui_ImplPhobos_GetTexture(ph::ImageView view);
void ImGui_ImplPhobos_RemoveTexture(ph::ImageView view);

#endif
