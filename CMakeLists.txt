cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(rendercat VERSION 0.1)

find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(glew 2 REQUIRED)
find_package(glm 0.9 REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(third_party)

add_executable(rendercat
	src/main.cpp
	src/mesh.cpp
	src/renderer.cpp
	src/scene.cpp
	src/shader_set.cpp
)

set_target_properties(rendercat
	PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_include_directories(rendercat
	PRIVATE
		${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(rendercat
	PRIVATE
		Boost::filesystem
		Stb::Stb
		tinyobjloader
		glfw
		glm
		${GLEW_LIBRARIES}
		${OPENGL_gl_LIBRARY}
)

if(CMAKE_COMPILER_IS_GNUCC)
	target_compile_options(rendercat
		PRIVATE
			"-Wall"
			"-Wextra"
			"-pedantic")

endif()

set(SHADERS_SOURCES
	"shaders/generic.frag"
	"shaders/generic.vert"
	"shaders/shadow_mapping.vert"
	"shaders/shadow_mapping.frag"
	"shaders/debug_quad.vert"
	"shaders/debug_quad.frag"
)

add_custom_target(shaders
	SOURCES
		${SHADERS_SOURCES}
	DEPENDS
		rendercat
)
