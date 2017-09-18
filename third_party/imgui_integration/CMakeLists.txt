project(imgui_integration VERSION 0.1)

set(imgui_srcs
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui_impl_glfw.cpp)

add_library(imgui ${imgui_srcs})

target_link_libraries(imgui glbinding::glbinding glfw)

target_compile_definitions(imgui PUBLIC
	IMGUI_DISABLE_WIN32_DEFAULT_CLIPBOARD_FUNCS
	IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCS
	IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)

target_include_directories(imgui PUBLIC include)
