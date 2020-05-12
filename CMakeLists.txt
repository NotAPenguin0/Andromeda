cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(andromeda)

enable_language(CXX)
# Andromeda targets C++20
set(CMAKE_CXX_STANDARD 20)

# This allows target_sources to use different relative paths if they are in a different directory than the target
cmake_policy(SET CMP0076 NEW)
# Allow calling target_link_libraries on targets not built in the same directory
cmake_policy(SET CMP0079 NEW)

set(BUILD_SHARED_LIBS OFF)

add_executable(andromeda "")
add_subdirectory("src")
add_subdirectory("external")
add_subdirectory("shaders")

if (WIN32 AND ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	# Currently, clang on windows does not support concepts, so we have to silence this warning/error
	target_compile_definitions(andromeda PUBLIC _SILENCE_CLANG_CONCEPTS_MESSAGE)
endif()

if (${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC")
	target_compile_options(andromeda PRIVATE -W3 -Werror)
endif()

if (WIN32)
    # Truly a classic
	target_compile_definitions(andromeda PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

target_include_directories(andromeda PRIVATE "include")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
	target_compile_definitions(andromeda PUBLIC ANDROMEDA_DEBUG=1)
endif()

# Precompiled headers 
target_precompile_headers(andromeda PUBLIC "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.hpp")