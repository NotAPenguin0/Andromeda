file(GLOB SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.vert" "${CMAKE_CURRENT_SOURCE_DIR}/*.frag" "${CMAKE_CURRENT_SOURCE_DIR}/*.comp")
set(SHADER_OUTPUT_FILES "")
foreach(SHADER ${SHADER_SOURCES})
	get_filename_component(SHADER_FNAME ${SHADER} NAME)
    set(SHADER_OUTPUT ${CMAKE_BINARY_DIR}/data/shaders/${SHADER_FNAME}.spv)
    list(APPEND SHADER_OUTPUT_FILES ${SHADER_OUTPUT})
	add_custom_command(OUTPUT ${SHADER_OUTPUT}
        COMMAND glslc ${SHADER} "-o${SHADER_OUTPUT}"
        DEPENDS ${SHADER}
    )
endforeach()

add_custom_target(andromeda-shaders
    ALL DEPENDS ${SHADER_OUTPUT_FILES}
    COMMENT "Building shaders"
)

add_dependencies(andromeda andromeda-shaders)