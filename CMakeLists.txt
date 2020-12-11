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

set(USE_DEBUG_TORCH 1)

if(USE_DEBUG_TORCH)
	set(TORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libtorch-debug")
	message("Using debug Torch")
else()
	set(TORCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libtorch-release")
	message("Using release Torch")
endif()

list(APPEND CMAKE_PREFIX_PATH "${TORCH_DIR}/share/cmake/Torch")

add_subdirectory(Strife.Common)
add_subdirectory(Strife.ML)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(OALWrapper)
add_subdirectory(ContentPipeline)
add_subdirectory(tmxlite/tmxlite)
