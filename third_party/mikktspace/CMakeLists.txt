project(mikktspace VERSION 1.0)

add_library(mikktspace_impl STATIC mikktspace.c)
add_library(mikktspace::mikktspace ALIAS mikktspace_impl)

target_include_directories(mikktspace_impl PUBLIC "${PROJECT_SOURCE_DIR}")
