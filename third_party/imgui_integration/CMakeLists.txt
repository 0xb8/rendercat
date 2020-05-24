cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(imgui_integration VERSION 0.1)

add_library(imgui STATIC
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_widgets.cpp
	imgui/examples/imgui_impl_glfw.cpp
	imgui/examples/imgui_impl_opengl3.cpp
	imgui/imgui.h
)

add_library(imgui::imgui ALIAS imgui)


target_link_libraries(imgui
	PRIVATE
		glbinding::glbinding
		glfw
)

set_target_properties(imgui PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON)

target_compile_definitions(imgui
	PUBLIC
		IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS
		IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS
		IMGUI_DISABLE_OBSOLETE_FUNCTIONS
	PRIVATE
		IMGUI_IMPL_OPENGL_LOADER_GLBINDING3
		GL_VERSION_3_2
)

target_include_directories(imgui
	PUBLIC
		"${PROJECT_SOURCE_DIR}/imgui"
	PRIVATE
		"${PROJECT_SOURCE_DIR}/imgui/examples")
