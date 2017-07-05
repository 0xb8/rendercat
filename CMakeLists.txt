cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(learngl VERSION 0.1)

find_package(Boost COMPONENTS system filesystem REQUIRED)

add_subdirectory(third_party)

add_executable(glrun
	src/main.cpp
	src/renderer.cpp
	src/scene.cpp
	src/shader_set.cpp
)

set_target_properties(glrun PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_include_directories(glrun PRIVATE ${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(glrun PRIVATE ${Boost_LIBRARIES} stb glfw3 glew32 gdi32 opengl32 )


add_custom_target(copy-runtime-files ALL
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_BINARY_DIR}/shaders
    COMMAND cmake -E copy_directory ${CMAKE_SOURCE_DIR}/assets  ${CMAKE_BINARY_DIR}/assets
    DEPENDS glrun)


add_custom_target(shaders SOURCES "shaders/triangle.frag" "shaders/triangle.vert" DEPENDS glrun)






