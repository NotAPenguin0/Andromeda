set(VALID_SHADER_SOURCE_EXTENSIONS ".vert" ".frag" ".comp" ".rgen" ".rahit" ".rchit" ".rmiss")
set(SHADER_FILTER "")
foreach(EXTENSION ${VALID_SHADER_SOURCE_EXTENSIONS})
    list(APPEND SHADER_FILTER "${CMAKE_CURRENT_SOURCE_DIR}/*${EXTENSION}")
endforeach()

# Grab all shader source files
file(GLOB SHADER_SOURCES ${SHADER_FILTER})
file(GLOB SHADER_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/*.glsl")

set(SHADER_OUTPUT_FILES "")
foreach(SHADER ${SHADER_SOURCES})
	get_filename_component(SHADER_FNAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${CMAKE_BINARY_DIR}/data/shaders/${SHADER_FNAME}.spv)
    list(APPEND SHADER_OUTPUT_FILES ${SHADER_OUTPUT})
	add_custom_command(OUTPUT ${SHADER_OUTPUT}
        COMMAND glslc ${SHADER} "-o${SHADER_OUTPUT}" "--target-env=vulkan1.2"
        DEPENDS ${SHADER} ${SHADER_INCLUDES}
    )
endforeach()

add_custom_target(andromeda-shaders
    ALL DEPENDS ${SHADER_OUTPUT_FILES}
    COMMENT "Building shaders"
)

add_dependencies(andromeda andromeda-shaders)