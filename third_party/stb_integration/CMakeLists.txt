cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(stb)

add_library(stb_image
	stb_image.c
)
add_library(stb_image_write
	stb_image_write.c
)
add_library(stb::image ALIAS stb_image)
add_library(stb::image_write ALIAS stb_image_write)

target_include_directories(stb_image
	PUBLIC
		"${PROJECT_SOURCE_DIR}/stb"
)
target_include_directories(stb_image_write
	PUBLIC
		"${PROJECT_SOURCE_DIR}/stb"
)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_compile_options(stb_image PRIVATE -O2)
	target_compile_options(stb_image_write PRIVATE -O2)
endif()
