# Andromeda uses a custom renderer built on top of Phobos, so we don't want to build the phobos fixed pipeline
set(PHOBOS_NO_FIXED_PIPELINE ON)
add_subdirectory("phobos")
add_subdirectory("zlib")
add_subdirectory("imgui")


# Create target for imgui implementation files
add_library(imgui-implementation STATIC "")
target_sources(imgui-implementation PRIVATE "imgui/src/imgui_impl_mimas.cpp" "imgui/src/imgui_impl_phobos.cpp")
target_link_libraries(imgui-implementation PRIVATE Phobos)
target_include_directories(imgui-implementation PRIVATE "phobos/include" "imgui/include" "imgui/include/imgui")

target_link_libraries(andromeda PRIVATE Phobos zlibstatic imgui imgui-implementation)
target_include_directories(andromeda PRIVATE 
	"phobos/include" 
	"${CMAKE_CURRENT_SOURCE_DIR}/zlib"
	"imgui/include"
)


# zlib generates a zconf.h file in its build directory, so we need this available
target_include_directories(zlibstatic PUBLIC "${CMAKE_BINARY_DIR}/external/zlib")