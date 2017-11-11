cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(tinyheaders VERSION 0.1)

add_library(tinyfiles
	tinyfiles.cpp
)
add_library(tinyheaders::tinyfiles ALIAS tinyfiles)

target_include_directories(tinyfiles
	PUBLIC
		"${PROJECT_SOURCE_DIR}"
)

