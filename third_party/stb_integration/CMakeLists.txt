cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(stb)

add_library(stb_image STATIC stb_image.c stb_image_write.c)
add_library(stb::image ALIAS stb_image)
target_include_directories(stb_image PUBLIC "${PROJECT_SOURCE_DIR}/stb")
