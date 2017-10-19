cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(rendercat VERSION 0.1)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK    ccache)
endif()

SET(BUILD_SHARED_LIBS OFF)

find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(third_party)

add_executable(rendercat
	src/main.cpp
	src/material.cpp
	src/mesh.cpp
	src/renderer.cpp
	src/scene.cpp
	src/shader_set.cpp
	src/cubemap.cpp
	src/util/gl_perfquery.cpp
	src/util/gl_screenshot.cpp
)

set_target_properties(rendercat
	PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_include_directories(rendercat
	PUBLIC
		"${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(rendercat
	PRIVATE
		Boost::filesystem
		stb::image
		stb::image_write
		imgui::imgui
		glbinding::glbinding
		OpenGL::GL
		glfw
		glm
		tinyobjloader
		mikktspace
)

if(CMAKE_COMPILER_IS_GNUCC)
	target_compile_options(rendercat
		PRIVATE
			"-Wall"
			"-Wextra"
			"-pedantic")

endif()

add_subdirectory(shaders)
add_dependencies(rendercat shaders)

