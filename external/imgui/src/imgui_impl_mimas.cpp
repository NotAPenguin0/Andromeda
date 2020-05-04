// dear imgui: Platform Binding for Mimas
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan..)
// (Info: Mimas is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you are new to dear imgui, read examples/README.txt and read the documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include "imgui.h"
#include "imgui_impl_mimas.h"

// Mimas
#include <mimas/mimas.h>

// Mimas does not provide timing functions, so we have to implement this using std::chrono
#include <chrono>
#include <algorithm>

static Mimas_Window* g_Window = nullptr;
static long long g_Time = 0;
static bool g_MouseJustPressed[3] = {false, false, false};

// Old user callbacks
static mimas_window_key_callback g_PrevUserCallbackKey = nullptr;
static mimas_window_mouse_button_callback g_PrevUserCallbackMouseButton = nullptr;
static mimas_window_scroll_callback g_PrevUserCallbackScroll = nullptr;


static void* g_PrevUserCallbackKeyData = nullptr;
static void* g_PrevUserCallbackMouseButtonData = nullptr;
static void* g_PrevUserCallbackScrollData = nullptr;

static void ImGui_ImplMimas_KeyCallback(Mimas_Window* window, Mimas_Key key, 
    Mimas_Key_Action action, void* user_data) {
    
    if (g_PrevUserCallbackKey) {
        g_PrevUserCallbackKey(window, key, action, g_PrevUserCallbackKeyData);
    }

    ImGuiIO& io = ImGui::GetIO();
    if  (action == MIMAS_KEY_PRESS) {
        io.KeysDown[key] = true;
    } else if (action == MIMAS_KEY_RELEASE) {
        io.KeysDown[key] = false;
    }

    // Modifiers are not reliable across systems.
    // These are not yet implemented in mimas
//     io.KeyCtrl = io.KeysDown[MIMAS_KEY_L] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
//     io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
//     io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
// #ifdef _WIN32
//     io.KeySuper = false;
// #else
//     io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
// #endif
}

static void ImGui_ImplMimas_MouseButtonCallback(Mimas_Window* window, Mimas_Key btn, 
    Mimas_Mouse_Button_Action action, void* user_data) {

    if (g_PrevUserCallbackMouseButton) {
        g_PrevUserCallbackMouseButton(window, btn, action, g_PrevUserCallbackMouseButtonData);
    }

    if (action == MIMAS_MOUSE_BUTTON_PRESS) {
        if (btn >= 0 && btn < IM_ARRAYSIZE(g_MouseJustPressed)) {
            g_MouseJustPressed[btn] = true;
        }
    }
}

static void ImGui_ImplMimas_ScrollCallback(Mimas_Window* window, mimas_i32 x, mimas_i32 y, void*) {
    if (g_PrevUserCallbackScroll) {
        g_PrevUserCallbackScroll(window, x, y, g_PrevUserCallbackScrollData);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)x;
    io.MouseWheel += (float)y;
}

bool ImGui_ImplMimas_InitForVulkan(Mimas_Window* window) {
    g_Window = window;
    g_Time = 0;
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "imgui_impl_mimas";

    // Missing keys in current mimas version
    io.KeyMap[ImGuiKey_Tab] = MIMAS_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = MIMAS_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = MIMAS_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = MIMAS_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = MIMAS_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = MIMAS_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = MIMAS_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = MIMAS_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = MIMAS_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = MIMAS_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = MIMAS_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = MIMAS_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = MIMAS_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = MIMAS_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = MIMAS_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = MIMAS_KEY_NUMPAD_ENTER;
    io.KeyMap[ImGuiKey_A] = MIMAS_KEY_A;
    io.KeyMap[ImGuiKey_C] = MIMAS_KEY_C;
    io.KeyMap[ImGuiKey_V] = MIMAS_KEY_V;
    io.KeyMap[ImGuiKey_X] = MIMAS_KEY_X;
    io.KeyMap[ImGuiKey_Y] = MIMAS_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = MIMAS_KEY_Z;
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)g_Window;

    Mimas_Callback prev_key_cb = mimas_get_window_key_callback(window);
    Mimas_Callback prev_mouse_button_cb = mimas_get_window_mouse_button_callback(window);
    Mimas_Callback prev_scroll_cb = mimas_get_window_scroll_callback(window);

    g_PrevUserCallbackKey = (mimas_window_key_callback)prev_key_cb.callback;
    g_PrevUserCallbackKeyData = prev_key_cb.user_data;
    g_PrevUserCallbackMouseButton = (mimas_window_mouse_button_callback)prev_mouse_button_cb.callback;
    g_PrevUserCallbackMouseButtonData = prev_mouse_button_cb.user_data;
    g_PrevUserCallbackScroll = (mimas_window_scroll_callback)prev_scroll_cb.callback;
    g_PrevUserCallbackScrollData = prev_scroll_cb.user_data;

    mimas_set_window_key_callback(window, ImGui_ImplMimas_KeyCallback, nullptr);
    mimas_set_window_mouse_button_callback(window, ImGui_ImplMimas_MouseButtonCallback, nullptr);
    mimas_set_window_scroll_callback(window, ImGui_ImplMimas_ScrollCallback, nullptr);
    
    return true;
}


static void ImGui_ImplMimas_UpdateMousePosAndButtons() {
    ImGuiIO& io = ImGui::GetIO();
    
    for (int i = 0; i < IM_ARRAYSIZE(g_MouseJustPressed); ++i) {
        bool button_down = mimas_get_key((Mimas_Key)((int)MIMAS_MOUSE_LEFT_BUTTON + i)) == MIMAS_MOUSE_BUTTON_PRESS;
        io.MouseDown[i] = g_MouseJustPressed[i] || button_down;
        g_MouseJustPressed[i] = false;
    }

    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseHoveredViewport = 0;
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 0; i < platform_io.Viewports.Size; ++i) {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        Mimas_Window* window = (Mimas_Window*)viewport->PlatformHandle;
        IM_ASSERT(window != nullptr);
        // TODO: Check if window is focused
        bool const focused = true;
        if (focused) {
            if (io.WantSetMousePos) {
                // Missing mimas feature: set cursor pos
            } else {
                mimas_i32 mouse_x, mouse_y;
                mimas_get_cursor_pos(&mouse_x, &mouse_y);
                mimas_i32 window_content_pos_x, window_content_pos_y;
                mimas_get_window_content_pos(window, &window_content_pos_x, &window_content_pos_y);
                // Multi-viewport mode. 
                // This is untested. 
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                    mimas_i32 window_x, window_y;
                    mimas_get_window_pos(window, &window_x, &window_y);
                    io.MousePos = ImVec2((float)mouse_x + window_x, (float)mouse_y + window_y);
                } else {
                    // Single viewport mode
                    io.MousePos = ImVec2((float)mouse_x - window_content_pos_x, (float)mouse_y - window_content_pos_y);
                }
            }
             for (int i = 0; i < IM_ARRAYSIZE(g_MouseJustPressed); ++i) {
                io.MouseDown[i] |= (mimas_get_key((Mimas_Key)((int)MIMAS_MOUSE_LEFT_BUTTON + i)) == MIMAS_MOUSE_BUTTON_PRESS);
            }
        }
    }
}

void ImGui_ImplMimas_NewFrame() {
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");
    int display_w, display_h;
    mimas_get_window_content_size(g_Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)display_w, (float)display_h);
    // TODO
    io.DisplayFramebufferScale = ImVec2(1, 1);

    // Update deltatime
    auto time = mimas_get_time();
    io.DeltaTime = 1.0f / 60.0f; // Make ImGui run at 60 fps
    g_Time = time;
    ImGui_ImplMimas_UpdateMousePosAndButtons();
}

void ImGui_ImplMimas_Shutdown() {
    // Reinstall old callbacks
    mimas_set_window_key_callback(g_Window, g_PrevUserCallbackKey, g_PrevUserCallbackKeyData);
    mimas_set_window_mouse_button_callback(g_Window, g_PrevUserCallbackMouseButton, g_PrevUserCallbackMouseButtonData);
    mimas_set_window_scroll_callback(g_Window, g_PrevUserCallbackScroll, g_PrevUserCallbackScrollData);
}
