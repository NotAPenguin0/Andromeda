include(FetchContent)

# 'option()' honors normal variables
cmake_policy(SET CMP0077 NEW)

FetchContent_Declare(
	phobos
	GIT_REPOSITORY https://github.com/NotAPenguin0/Phobos
)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw
)

FetchContent_Declare(
	colorconsole
	GIT_REPOSITORY https://github.com/imfl/color-console
)

set(PHOBOS_ENABLE_RAY_TRACING ON)

FetchContent_MakeAvailable(phobos)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(colorconsole)

target_link_libraries(andromeda PRIVATE Phobos glfw)

target_include_directories(andromeda PRIVATE "${colorconsole_SOURCE_DIR}/include")