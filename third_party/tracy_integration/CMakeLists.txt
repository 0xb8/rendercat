cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(tracy)

add_library(tracy STATIC
	tracy/TracyClient.cpp
	tracy/Tracy.hpp
	tracy/TracyOpenGL.hpp
	TracyOpenGL.hpp)

add_library(tracy::tracy ALIAS tracy)
target_include_directories(tracy PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(tracy PUBLIC $<$<PLATFORM_ID:Windows>:Ws2_32 Dbghelp>)
target_compile_definitions(tracy PRIVATE TRACY_NO_BROADCAST TRACY_ONLY_LOCALHOST TRACY_ONLY_IPV4)
#if (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo)
	target_compile_definitions(tracy PUBLIC TRACY_ENABLE)
	message(STATUS "Tracy enabled")
#endif()
