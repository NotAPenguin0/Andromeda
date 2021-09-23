include(FetchContent)

# 'option()' honors normal variables
cmake_policy(SET CMP0077 NEW)

FetchContent_Declare(
	phobos
	GIT_REPOSITORY https://github.com/NotAPenguin0/Phobos
)

FetchContent_Declare(
	plib 
	GIT_REPOSITORY https://github.com/NotAPenguin0/plib
)

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw
)

FetchContent_Declare(
	colorconsole
	GIT_REPOSITORY https://github.com/NotAPenguin0/color-console
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

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm
)

FetchContent_Declare( 
	iconfonts 
	GIT_REPOSITORY https://github.com/NotAPenguin0/IconFontCppHeaders
	GIT_TAG e4c6b0e7bbba588e15195315a2f754e48cec3de1 # branch is named 'main', not 'master'
)

FetchContent_Declare( 
	simplejson
	GIT_REPOSITORY https://github.com/NotAPenguin0/SimpleJSON
)

#set(PHOBOS_ENABLE_RAY_TRACING ON)

FetchContent_MakeAvailable(phobos)
FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(colorconsole)
FetchContent_MakeAvailable(assetlib)
FetchContent_MakeAvailable(imgui_docking)
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(iconfonts)
FetchContent_MakeAvailable(plib)
FetchContent_MakeAvailable(simplejson)

target_link_libraries(andromeda PRIVATE Phobos glfw assetlib imgui plib)

target_include_directories(andromeda PRIVATE
	"${colorconsole_SOURCE_DIR}/include" 
	"${imgui_docking_SOURCE_DIR}/include"
	"${glm_SOURCE_DIR}"
	"${iconfonts_SOURCE_DIR}/include"
	"${simplejson_SOURCE_DIR}/include"
)