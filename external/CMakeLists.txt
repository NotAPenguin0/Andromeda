# Andromeda uses a custom renderer built on top of Phobos, so we don't want to build the phobos fixed pipeline
set(PHOBOS_NO_FIXED_PIPELINE ON)
add_subdirectory("phobos")
add_subdirectory("zlib")

target_link_libraries(andromeda PRIVATE Phobos zlibstatic)
target_include_directories(andromeda PRIVATE "phobos/include" "${CMAKE_CURRENT_SOURCE_DIR}/zlib")

# zlib generates a zconf.h file in its build directory, so we need this available
target_include_directories(zlibstatic PUBLIC "${CMAKE_BINARY_DIR}/external/zlib")