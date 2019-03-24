cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(rendercat VERSION 0.1)
cmake_policy(SET CMP0077 NEW)

#SET(BUILD_SHARED_LIBS ON)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# use thin archives for speed, don't update index (ranlib will do it afterwards)
	set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> cruTS <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> ruTS  <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> cruTS <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_APPEND   "<CMAKE_AR> ruTS  <TARGET> <LINK_FLAGS> <OBJECTS>")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	# use LTO in release (GCC only for now - Clang hangs on glbinding)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
endif()


find_package(OpenGL REQUIRED)

add_subdirectory(third_party)

add_executable(rendercat
	src/cubemap.cpp
	src/main.cpp
	src/material.cpp
	src/mesh.cpp
	src/renderer.cpp
	src/scene.cpp
	src/shader_set.cpp
	src/texture.cpp
	src/texture2d.cpp
	src/texture_cache.cpp
	src/core/bbox.cpp
	src/core/camera.cpp
	src/core/frustum.cpp
	src/util/color_temperature.cpp
	src/util/gl_meta.cpp
	src/util/gl_perfquery.cpp
	src/util/gl_screenshot.cpp
	src/util/gl_unique_handle.cpp
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
		fmt::fmt
		debug::draw
		glbinding::glbinding
		glbinding::glbinding-aux
		glfw
		zcm
		imgui::imgui
		mikktspace::mikktspace
		OpenGL::GL
		stb::image
		tinyheaders::tinyfiles
		tinyobjloader
		fx-gltf
)

if(CMAKE_COMPILER_IS_GNUCC)
	set_property(
		SOURCE src/mesh.cpp
		APPEND PROPERTY COMPILE_OPTIONS $<$<CONFIG:DEBUG>:-O3 -g0>)
	target_compile_options(rendercat
		PRIVATE
			"-Wall"
			"-Wextra"
			"-pedantic")

endif()

add_subdirectory(shaders)
add_dependencies(rendercat shaders)

option(RC_BUILD_TESTS "Enable test building" ON)
if(RC_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()

