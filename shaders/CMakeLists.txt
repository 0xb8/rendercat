project(rendercat_shaders VERSION 0.1)

set(VERTEX_SHADER_SOURCES
	generic.vert
	cubemap.vert
	fullscreen_quad.vert
	shadow_mapping.vert
)

set(FRAGMENT_SHADER_SOURCES
	generic.frag
	cubemap.frag
	hdr.frag
	shadow_mapping.frag
)

set(COMPUTE_SHADER_SOURCES
	brdf_lut.comp)

source_group("Vertex Shaders" FILES ${VERTEX_SHADER_SOURCES})
source_group("Fragment Shaders" FILES ${FRAGMENT_SHADER_SOURCES})

find_program(GLSLANG_EXECUTABLE glslangValidator)
if(NOT GLSLANG_EXECUTABLE)
	message(FATAL_ERROR "Could not find glslangValidator program. Please install 'glslang' package.")
endif()

add_custom_target(shaders
	SOURCES
		${VERTEX_SHADER_SOURCES}
		${FRAGMENT_SHADER_SOURCES}
		${COMPUTE_SHADER_SOURCES}
	COMMAND ${GLSLANG_EXECUTABLE} ${VERTEX_SHADER_SOURCES} ${FRAGMENT_SHADER_SOURCES} ${COMPUTE_SHADER_SOURCES}
	COMMENT "Validating shader sources"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
