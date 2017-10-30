project(tinyheaders_integration)

add_library(tinyfiles_impl
	tinyfiles.cpp
)
add_library(tinyheaders::tinyfiles ALIAS tinyfiles_impl)

target_include_directories(tinyfiles_impl
	PUBLIC
		"${PROJECT_SOURCE_DIR}"
)

