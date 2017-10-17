project(stb_integration)

add_library(stb_image_impl
	stb_image.c
)
add_library(stb_image_write_impl
	stb_image_write.c
)
add_library(stb::image ALIAS stb_image_impl)
add_library(stb::image_write ALIAS stb_image_write_impl)

target_include_directories(stb_image_impl
	PUBLIC
		"${PROJECT_SOURCE_DIR}/stb"
)
target_include_directories(stb_image_write_impl
	PUBLIC
		"${PROJECT_SOURCE_DIR}/stb"
)

if(CMAKE_COMPILER_IS_GNUCXX)
	target_compile_options(stb_image_impl PRIVATE -O2)
	target_compile_options(stb_image_write_impl PRIVATE -O2)
endif()

