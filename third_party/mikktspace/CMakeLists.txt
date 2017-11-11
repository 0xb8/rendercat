cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(mikktspace VERSION 1.0)

add_library(mikktspace STATIC mikktspace.c)
add_library(mikktspace::mikktspace ALIAS mikktspace)

target_include_directories(mikktspace PUBLIC "${PROJECT_SOURCE_DIR}")
