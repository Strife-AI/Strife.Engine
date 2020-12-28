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

if (MSVC)
	if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		if (NOT DEFINED ENV{TORCH_DEBUG_DIR})
			message(FATAL_ERROR "Environment variable TORCH_DEBUG_DIR not set to location of debug torch")
		endif()

		set(TORCH_DIR $ENV{TORCH_DEBUG_DIR} CACHE STRING "Path to torch")
	else()
		if (NOT DEFINED ENV{TORCH_RELEASE_DIR})
			message(FATAL_ERROR "Environment variable TORCH_RELEASE_DIR not set to location of release torch")
		endif()

		set(TORCH_DIR $ENV{TORCH_RELEASE_DIR} CACHE STRING "Path to torch")
	endif()

	list(APPEND CMAKE_PREFIX_PATH "${TORCH_DIR}/share/cmake/Torch")
endif()

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(Strife.Common)
add_subdirectory(Strife.ML)
add_subdirectory(src)
add_subdirectory(OALWrapper)
add_subdirectory(ContentPipeline)
add_subdirectory(tmxlite/tmxlite)
