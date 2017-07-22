cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(rendercat VERSION 0.1)

find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(glfw3 3.2 REQUIRED)
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


#add_custom_target(copy-runtime-files ALL
#    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
#    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/assets  ${CMAKE_BINARY_DIR}/assets
#    DEPENDS rendercat)


add_custom_target(shaders
	SOURCES
		"shaders/generic.frag"
		"shaders/generic.vert"
		"shaders/point_lamp.vert"
		"shaders/point_lamp.frag"
	DEPENDS
		rendercat
)


