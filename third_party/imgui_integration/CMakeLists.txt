cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(imgui_integration VERSION 0.1)

add_library(imgui_impl
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui_impl_glfw.cpp
)

add_library(imgui::imgui ALIAS imgui_impl)


target_link_libraries(imgui_impl
	PRIVATE
		glbinding::glbinding
		glfw
)

target_compile_definitions(imgui_impl
	PUBLIC
		IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS
		IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS
		IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)

target_include_directories(imgui_impl PUBLIC "${PROJECT_SOURCE_DIR}/include")
