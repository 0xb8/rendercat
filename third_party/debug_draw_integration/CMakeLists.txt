cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(debug-draw VERSION 0.1)

add_library(debug_draw STATIC
    debug_draw.cpp
    debug_draw.hpp
    debug_draw_interface.hpp
    debug_draw_impl.cpp
    )
add_library(debug::draw ALIAS debug_draw)

target_link_libraries(debug_draw
	PRIVATE
		glbinding::glbinding
	PUBLIC
		zcm::zcm
)

set_target_properties(debug_draw
	PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
)

target_include_directories(debug_draw PUBLIC "${PROJECT_SOURCE_DIR}")
