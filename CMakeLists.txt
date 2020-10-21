cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(Engine VERSION 1 LANGUAGES C CXX)

# Enable exception unwind semantics
add_compile_options("/EHsc")
add_compile_options("/MP")

add_subdirectory(src)