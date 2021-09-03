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

set(ANDROMEDA_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory("external")
add_subdirectory("src")
add_subdirectory("shaders")

add_subdirectory("codegen")

target_include_directories(andromeda PUBLIC ${ANDROMEDA_INCLUDE_DIRECTORIES})


if (${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC")
	target_compile_options(andromeda PRIVATE -W3 -Werror)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") 
    message("Using clang to compile")
	target_compile_options(andromeda PRIVATE -W3)
endif()

if (WIN32)
    # Truly a classic
	target_compile_definitions(andromeda PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	target_compile_definitions(andromeda PUBLIC ANDROMEDA_DEBUG=1)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
	target_compile_definitions(andromeda PUBLIC ANDROMEDA_DEBUG=1)
endif()
