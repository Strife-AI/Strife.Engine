cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(Engine VERSION 1 LANGUAGES C CXX)

if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

# Enable exception unwind semantics
if(MSVC)
	add_compile_options("/EHsc")
	add_compile_options("/MP")

	set(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0 -DNOGDI -DWIN32")
elseif(LINUX)
	#set(CMAKE_CXX_FLAGS "-lasan -fsanitize=address")
elseif(APPLE)
	set(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0")
endif()

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(Strife.ML)
add_subdirectory(src)
add_subdirectory(OALWrapper)
add_subdirectory(tmxlite/tmxlite)
