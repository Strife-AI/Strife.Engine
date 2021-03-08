cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_library(Strife.ML STATIC
 "NewStuff.hpp" "NewStuff.cpp" TensorPacking.hpp TorchApi.h TorchApi.cpp TorchApiInternal.hpp)
	
find_package(Microsoft.GSL CONFIG REQUIRED)
	
find_package(torch REQUIRED)
target_link_libraries(Strife.ML PUBLIC torch Microsoft.GSL::GSL Strife.Common)

set_property(TARGET Strife.ML PROPERTY CXX_STANDARD 17)