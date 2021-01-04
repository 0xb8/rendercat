cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(imgui_integration VERSION 0.1)

add_library(imgui STATIC
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/backends/imgui_impl_glfw.cpp
	imgui/backends/imgui_impl_opengl3.cpp
	imgui/imgui.h
	imgui/misc/cpp/imgui_stdlib.cpp
	imguizmo/ImGuizmo.h
	imguizmo/ImGuizmo.cpp
	implot/implot.h
	implot/implot.cpp
	implot/implot_demo.cpp
	implot/implot_items.cpp
)

add_library(imgui::imgui ALIAS imgui)


target_link_libraries(imgui
	PUBLIC
		zcm::zcm
	PRIVATE
		glbinding::glbinding
		glfw
)

set_target_properties(imgui PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF)

target_compile_definitions(imgui
	PRIVATE
		IMGUI_IMPL_OPENGL_LOADER_GLBINDING3
		GL_VERSION_3_2
)

target_include_directories(imgui
	PUBLIC
		"${PROJECT_SOURCE_DIR}"
	PRIVATE
		"${PROJECT_SOURCE_DIR}/imgui"
		"${PROJECT_SOURCE_DIR}/implot"
		"${PROJECT_SOURCE_DIR}/imgui/examples")
