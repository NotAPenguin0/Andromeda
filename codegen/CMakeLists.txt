include(FetchContent)

# Create codegen executable
add_executable(codegen "")
target_sources(codegen PRIVATE 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/parse.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/generate.cpp"
)

target_include_directories(codegen PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

FetchContent_Declare(
	cppast-lib
	GIT_REPOSITORY https://github.com/foonathan/cppast
	GIT_TAG 032d48df2ed85ee3984ef317d40e4b14d83198be # master branch is named main
)

FetchContent_Declare( 
	mustache 
	GIT_REPOSITORY https://github.com/kainjow/Mustache
)

FetchContent_MakeAvailable(cppast-lib mustache)

target_link_libraries(codegen PRIVATE cppast)
target_include_directories(codegen PRIVATE ${mustache_SOURCE_DIR})

set(CODEGEN_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out")
set(CODEGEN_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include/andromeda/components")
set(CODEGEN_TEMPLATE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/templates")

# Define list of output files. This only needs to include headers.
set(CODEGEN_OUTPUT_FILES "")
list(APPEND CODEGEN_OUTPUT_FILES "${CODEGEN_OUTPUT_DIR}/include/reflect/type_lists.hpp")
list(APPEND CODEGEN_OUTPUT_FILES "${CODEGEN_OUTPUT_DIR}/include/reflect/component_reflect_decl.hpp")

# Define list of output sources. These will automatically be added to the list of output files.
set(CODEGEN_OUTPUT_SOURCES "")
list(APPEND CODEGEN_OUTPUT_SOURCES "${CODEGEN_OUTPUT_DIR}/src/reflection.cpp")

# Add sources to output file list.
foreach(SRC ${CODEGEN_OUTPUT_SOURCES})
	list(APPEND CODEGEN_OUTPUT_FILES "${SRC}")
endforeach()

# Gather dependencies for code generation program. This is all the files in the input directory, and all the template files.
set(CODEGEN_DEPENDENCIES "")
file(GLOB CODEGEN_DEPENDENCIES "${CODEGEN_INPUT_DIR}/*.hpp")

# By convention, all template files will end in .tpl
set(CODEGEN_TEMPLATES "")
file(GLOB CODEGEN_TEMPLATES ${CODEGEN_TEMPLATE_DIR}/".tpl")


# Create custom cmake command to execute the code generation program
set(CODEGEN_BINARY "${CMAKE_CURRENT_BINARY_DIR}/codegen")
add_custom_command(
	OUTPUT ${CODEGEN_OUTPUT_FILES}
	COMMAND ${CODEGEN_BINARY} "${CODEGEN_INPUT_DIR}" "${CODEGEN_OUTPUT_DIR}" "${CODEGEN_TEMPLATE_DIR}" "${ANDROMEDA_INCLUDE_DIRECTORIES}"
	DEPENDS ${CODEGEN_DEPENDENCIES} ${CODEGEN_TEMPLATES}
)

# Create target to execute code generation command.
add_custom_target(andromeda-do-codegen
	ALL DEPENDS ${CODEGEN_OUTPUT_FILES} ${CODEGEN_TEMPLATES}
	COMMENT "Generating reflection information"
)

# Create library for andromeda to link to with codegen sources
add_library(andromeda-codegen-lib STATIC)
target_sources(andromeda-codegen-lib PRIVATE ${CODEGEN_OUTPUT_SOURCES})
target_include_directories(andromeda-codegen-lib PUBLIC "${CODEGEN_OUTPUT_DIR}/include")
target_include_directories(andromeda-codegen-lib PRIVATE ${ANDROMEDA_INCLUDE_DIRECTORIES})

# Setup manual target dependencies.
add_dependencies(andromeda-do-codegen codegen)
add_dependencies(andromeda-codegen-lib andromeda-do-codegen)
add_dependencies(andromeda codegen)
add_dependencies(andromeda andromeda-do-codegen)

# Link main executable to our generated reflection library
target_link_libraries(andromeda PRIVATE andromeda-codegen-lib)