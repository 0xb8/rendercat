project(stb_integration)

add_library(stb-image stb_image.c)
target_include_directories(stb-image PUBLIC "${PROJECT_SOURCE_DIR}/stb")
if(CMAKE_COMPILER_IS_GNUCXX)
	target_compile_options(stb-image PRIVATE -O2)
endif()

