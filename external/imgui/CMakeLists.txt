add_library(imgui STATIC "")
target_sources(imgui PRIVATE "src/imgui.cpp" "src/imgui_demo.cpp" "src/imgui_draw.cpp" "src/imgui_widgets.cpp")
# imgui sources all use #include <imgui.h> instead of imgui/imgui.h, so we have to do this
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui")
