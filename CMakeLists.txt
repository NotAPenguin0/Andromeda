cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(andromeda C CXX)

set(CMAKE_CXX_STANDARD 20)

# Additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# This allows target_sources to use different relative paths if they are in a different directory than the target
cmake_policy(SET CMP0076 NEW)
# Allow calling target_link_libraries on targets not built in the same directory
cmake_policy(SET CMP0079 NEW)

# We prefer static libraries
set(BUILD_SHARED_LIBS OFF)

add_executable(andromeda "")

target_include_directories(andromeda PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_SOURCE_DIR}/shaders/include"
		)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -EHa")

add_subdirectory("external")
add_subdirectory("src")
add_subdirectory("shaders")

add_subdirectory("codegen")

# Copy over build data
file(GLOB DATA_FILES "build/data/*")
add_custom_command(
	TARGET andromeda POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/build/data/" $<TARGET_FILE_DIR:andromeda>/data
	DEPENDS ${DATA_FILES}
	COMMENT "Copying build data"
	VERBATIM
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	message(STATUS "Using MSVC config")
	target_compile_options(andromeda PUBLIC -W3 -WX) # -WX is -Werror
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Clang)|(GNU)")
	message(STATUS "Using Clang/GNU config")
	target_compile_options(andromeda PUBLIC
			-Wall -Wpedantic -Werror -Wno-attributes -Wno-nullability-completeness
			-fcxx-exceptions
			)
else()
	message(FATAL_ERROR "Unknown compiler with ID ${CMAKE_CXX_COMPILER_ID}")
endif()

if (WIN32)
    # Truly a classic
	target_compile_definitions(andromeda PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

target_compile_definitions(andromeda PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	target_compile_definitions(andromeda PUBLIC ANDROMEDA_DEBUG=1)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
	target_compile_definitions(andromeda PUBLIC ANDROMEDA_DEBUG=1)
endif()
