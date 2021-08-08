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

FetchContent_Declare(
	assetlib
	GIT_REPOSITORY https://github.com/notAPenguin0/assetlib
)

FetchContent_Declare(
	imgui_docking
	GIT_REPOSITORY https://github.com/NotAPenguin0/imgui/
	GIT_TAG f287d868bacefc1fe044ec834f9bd879d8659d17 # Latest commit on docking branch
)

#set(PHOBOS_ENABLE_RAY_TRACING ON)

FetchContent_MakeAvailable(phobos)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(colorconsole)
FetchContent_MakeAvailable(assetlib)
FetchContent_MakeAvailable(imgui_docking)

target_link_libraries(andromeda PRIVATE Phobos glfw assetlib imgui)

target_include_directories(andromeda PRIVATE 
	"${colorconsole_SOURCE_DIR}/include" 
	"${imgui_docking_SOURCE_DIR}/include"
)