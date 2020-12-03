cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_library(Strife.Common STATIC
	Thread/SpinLock.cpp
	Thread/ThreadPool.cpp
 "Math/BitMath.cpp" "Math/BitMath.hpp" "Math/Vector2.cpp" "Math/Vector2.hpp" "Math/Vector3.hpp" "Math/Vector4.hpp" "Container/Grid.hpp")
	
find_package(Microsoft.GSL CONFIG REQUIRED)

target_include_directories(Strife.Common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
	
set_property(TARGET Strife.Common PROPERTY CXX_STANDARD 17)