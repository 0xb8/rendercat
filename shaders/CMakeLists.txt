project(rendercat_shaders VERSION 0.1)

set(VERTEX_SHADER_SOURCES
	generic.vert
	cubemap.vert
	fullscreen_triangle.vert
	shadow_mapping.vert
)

set(FRAGMENT_SHADER_SOURCES
	generic.frag
	cubemap.frag
	hdr.frag
	shadow_mapping.frag
)

set(COMPUTE_SHADER_SOURCES
	brdf_lut.comp
	cubemap_from_equirectangular.comp
	cubemap_diffuse_irradiance.comp
	cubemap_specular_envmap.comp
	downscale_bloom_luma.comp
	fsr_pass1_easu.comp
	fsr_pass2_rcas.comp
)

file(GLOB_RECURSE GLSL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.glsl" "*.h")
source_group("GLSL files" FILES ${GLSL_FILES})
add_custom_target(shaders-includes SOURCES ${GLSL_FILES})

source_group("Vertex Shaders" FILES ${VERTEX_SHADER_SOURCES})
source_group("Fragment Shaders" FILES ${FRAGMENT_SHADER_SOURCES})
source_group("Compute Shaders" FILES ${COMPUTE_SHADER_SOURCES})

set(SHADER_ALL_SOURCES
	${VERTEX_SHADER_SOURCES}
	${FRAGMENT_SHADER_SOURCES}
	${COMPUTE_SHADER_SOURCES})

find_program(GLSLC_EXECUTABLE glslc HINTS "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/shaderc/glslc")
if(NOT GLSLC_EXECUTABLE)
	message(STATUS "Could not find glslc. Please install 'shaderc' package.")
	add_custom_target(shaders SOURCES ${SHADER_ALL_SOURCES})

else()
	add_custom_target(shaders
		SOURCES ${SHADER_ALL_SOURCES}
		COMMAND ${GLSLC_EXECUTABLE} -Iinclude -c --target-env=opengl --target-spv=spv1.3 ${SHADER_ALL_SOURCES}
		COMMENT "Validating shader sources"
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
endif()
