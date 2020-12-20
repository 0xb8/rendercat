
add_executable(${PROJECT_NAME}
	cubemap.cpp
	cubemap.hpp
	main.cpp
	material.cpp
	material.hpp
	mesh.cpp
	mesh.hpp
	renderer.cpp
	renderer.hpp
	scene.cpp
	scene.hpp
	shader_set.cpp
	shader_set.hpp
	texture.cpp
	texture.hpp
	texture2d.cpp
	texture2d.hpp
	texture_cache.cpp
	texture_cache.hpp
	uniform.hpp
	uniform.cpp
	core/bbox.cpp
	core/bbox.hpp
	core/camera.cpp
	core/camera.hpp
	core/point_light.hpp
	common.hpp
	core/frustum.cpp
	core/frustum.hpp
	util/color_temperature.cpp
	util/color_temperature.hpp
	util/gl_meta.cpp
	util/gl_meta.hpp
	util/gl_perfquery.cpp
	util/gl_perfquery.hpp
	util/gl_screenshot.cpp
	util/gl_screenshot.hpp
	util/gl_unique_handle.cpp
	util/gl_unique_handle.hpp
)

set_target_properties(${PROJECT_NAME}
	PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		fmt::fmt
		debug::draw
		glbinding::glbinding
		glbinding::glbinding-aux
		glfw
		zcm::zcm
		imgui::imgui
		mikktspace::mikktspace
		OpenGL::GL
		stb::image
		fx-gltf::fx-gltf
		doctest::doctest
)

target_precompile_headers(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:rendercat/CMakePCH.h>)

if(BUILD_TESTING)
	enable_testing()
	add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME} -e)
else()
	target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_DISABLE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES Clang|GNU)
#	set_property(
#		SOURCE mesh.cpp
#		APPEND PROPERTY COMPILE_OPTIONS $<$<CONFIG:DEBUG>:-O3 -g0>)
	target_compile_options(${PROJECT_NAME}
		PRIVATE
			"-Wall"
			"-Wextra"
			"-Wpedantic")

endif()
