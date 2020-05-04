// Imgui platform binding for Mimas
// Has to be used with a renderer

// Implemented features
//
//
//
//
//


// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#pragma once

struct Mimas_Window;

IMGUI_IMPL_API bool ImGui_ImplMimas_InitForVulkan(Mimas_Window* window);

IMGUI_IMPL_API void ImGui_ImplMimas_NewFrame();

IMGUI_IMPL_API void ImGui_ImplMimas_Shutdown();

// TODO: Install callbacks