project(mikktspace VERSION 1.0)

add_library(mikktspace STATIC mikktspace.c)

target_include_directories(mikktspace PUBLIC "${PROJECT_SOURCE_DIR}")
