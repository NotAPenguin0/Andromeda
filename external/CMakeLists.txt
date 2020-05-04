# Andromeda uses a custom renderer built on top of Phobos, so we don't want to build the phobos fixed pipeline
set(PHOBOS_NO_FIXED_PIPELINE ON)
add_subdirectory("phobos")
# add_subdirectory("zlib")
add_subdirectory("imgui")

# Allow setting options created with option() through set()
cmake_policy(SET CMP0077 NEW)

set(ASSIMP_BUILD_ZLIB OFF)

set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_GLTF_EXPORTER ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)

add_subdirectory("assimp")

# Create target for imgui implementation files
add_library(imgui-implementation STATIC "")
target_sources(imgui-implementation PRIVATE "imgui/src/imgui_impl_mimas.cpp" "imgui/src/imgui_impl_phobos.cpp")
target_link_libraries(imgui-implementation PRIVATE Phobos)
target_include_directories(imgui-implementation PRIVATE "phobos/include" "imgui/include" "imgui/include/imgui")

target_compile_definitions(assimp PRIVATE -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
target_compile_options(assimp PRIVATE "-w")

target_link_libraries(andromeda PRIVATE Phobos imgui imgui-implementation assimp)
target_include_directories(andromeda PRIVATE 
	"phobos/include" 
	"${CMAKE_CURRENT_SOURCE_DIR}/zlib"
	"imgui/include"
	"assimp/include"
)


# zlib generates a zconf.h file in its build directory, so we need this available
target_include_directories(zlibstatic PUBLIC "${CMAKE_BINARY_DIR}/external/zlib")