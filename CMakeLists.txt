cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(Engine VERSION 1 LANGUAGES C CXX)

# Enable exception unwind semantics
add_compile_options("/EHsc")
add_compile_options("/MP")

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(OALWrapper)