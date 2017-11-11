cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(imgui_integration VERSION 0.1)

add_library(imgui
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui_impl_glfw.cpp
)

add_library(imgui::imgui ALIAS imgui)


target_link_libraries(imgui
	PRIVATE
		glbinding::glbinding
		glfw
)

target_compile_definitions(imgui
	PUBLIC
		IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS
		IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS
		IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)

target_include_directories(imgui PUBLIC "${PROJECT_SOURCE_DIR}")
