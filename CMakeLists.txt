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


if(CMAKE_CXX_COMPILER_ID MATCHES Clang|GNU AND NOT CMAKE_CXX_SIMULATE_ID)
	# use thin archives for speed, don't update index (ranlib will do it afterwards)
	set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> cruTS <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> ruTS  <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_CREATE   "<CMAKE_AR> cruTS <TARGET> <LINK_FLAGS> <OBJECTS>")
	set(CMAKE_C_ARCHIVE_APPEND   "<CMAKE_AR> ruTS  <TARGET> <LINK_FLAGS> <OBJECTS>")

	string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
	option(${PROJECT_NAME_UPPERCASE}_WITHOUT_LLD "Do not use LLD linker" OFF)
	option(${PROJECT_NAME_UPPERCASE}_WITHOUT_LLVM_AR "Do not use LLVM-AR archiver" OFF)
	if (CMAKE_BUILD_TYPE MATCHES Release AND CMAKE_CXX_COMPILER_ID MATCHES GNU)
		message(STATUS "Cannot use GCC LTO with LLD or LLVM-AR; disabling both.")
		set(${PROJECT_NAME_UPPERCASE}_WITHOUT_LLVM_AR ON)
		set(${PROJECT_NAME_UPPERCASE}_WITHOUT_LLD ON)
	endif()

	if (CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_SYSTEM_NAME MATCHES Windows)
		# on windows GNU ar(1) and ranlib(1) are very slow,
		# so attempt to find llvm-ar(1), which is much faster.
		find_program(LLVM_AR_PROGRAM llvm-ar)
		if (LLVM_AR_PROGRAM AND NOT ${PROJECT_NAME_UPPERCASE}_WITHOUT_LLVM_AR)
			message(STATUS "Using LLVM-AR as archiver and ranlib")
			set(CMAKE_AR "${LLVM_AR_PROGRAM}")
			set(CMAKE_RANLIB "${LLVM_AR_PROGRAM}")
			# llvm-ranlib does not recognize thin archives, use llvm-ar instead
			set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_AR> sT <TARGET>")
			set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_AR> sT <TARGET>")
		endif()
	endif()

	if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
		set(CMAKE_AR "${CMAKE_CXX_COMPILER_AR}")
		set(CMAKE_RANLIB "${CMAKE_CXX_COMPILER_RANLIB}")
		# llvm-ranlib does not recognize thin archives, use llvm-ar instead
		set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_AR> sT <TARGET>")
		set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_AR> sT <TARGET>")
	endif()

	# try to find and use lld(1), it's much faster than ld(1)
	find_program(LLD_PROGRAM ld.lld)
	if (LLD_PROGRAM AND NOT ${PROJECT_NAME_UPPERCASE}_WITHOUT_LLD)
		message(STATUS "Using LLD linker")
		set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
		set(${PROJECT_NAME_UPPERCASE}_WITH_LLD TRUE)
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
add_dependencies(${PROJECT_NAME} shaders-includes)
