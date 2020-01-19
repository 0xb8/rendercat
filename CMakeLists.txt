cmake_minimum_required(VERSION 3.16)
project(rendercat VERSION 0.1)
cmake_policy(SET CMP0077 NEW)

#SET(BUILD_SHARED_LIBS ON)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
include(CTest)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES Clang|GNU)
	# use thin archives for speed, don't update index (ranlib will do it afterwards)
	set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> cruTS <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> ruTS  <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> cruTS <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_APPEND   "<CMAKE_AR> ruTS  <TARGET> <LINK_FLAGS> <OBJECTS>")

	string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
	option(${PROJECT_NAME_UPPERCASE}_WITHOUT_LLD "Do not use LLD linker" OFF)

	if (NOT ${PROJECT_NAME_UPPERCASE}_WITHOUT_LLD)
		# use LLD linker
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
	endif()
	unset(PROJECT_NAME_UPPERCASE)

	# use LTO in release
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
endif()

# clang on mingw has problems with TLS destructors etc.
if (MINGW AND CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_definitions(-DDOCTEST_THREAD_LOCAL=)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -femulated-tls")
endif()

find_package(OpenGL REQUIRED)
option(USE_SYSTEM_GLBINDING "Use installed glbinding instead of subproject" ON)
if (USE_SYSTEM_GLBINDING)
	find_package(glbinding REQUIRED)
endif()

add_subdirectory(third_party)

add_subdirectory(rendercat)
add_subdirectory(shaders)
add_dependencies(${PROJECT_NAME} shaders)
