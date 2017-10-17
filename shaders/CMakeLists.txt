project(rendercat_shaders VERSION 0.1)

set(VERTEX_SHADER_SOURCES
	generic.vert
	cubemap.vert
	hdr.vert
	shadow_mapping.vert
)

set(FRAGMENT_SHADER_SOURCES
	generic.frag
	cubemap.frag
	hdr.frag
	shadow_mapping.frag
)

source_group("Vertex Shaders" FILES ${VERTEX_SHADER_SOURCES})
source_group("Fragment Shaders" FILES ${FRAGMENT_SHADER_SOURCES})


# TODO: check if validator exists
add_custom_target(shaders
	SOURCES
		${VERTEX_SHADER_SOURCES}
		${FRAGMENT_SHADER_SOURCES}
	COMMAND glslangValidator ${VERTEX_SHADER_SOURCES} ${FRAGMENT_SHADER_SOURCES}
	COMMENT "Validating shader sources"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)


