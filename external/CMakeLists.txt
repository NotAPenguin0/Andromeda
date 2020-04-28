# Andromeda uses a custom renderer built on top of Phobos, so we don't want to build the phobos fixed pipeline
set(PHOBOS_NO_FIXED_PIPELINE ON)
add_subdirectory("phobos")

target_link_libraries(andromeda PRIVATE Phobos)
target_include_directories(andromeda PRIVATE "phobos/include")