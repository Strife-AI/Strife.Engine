cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

add_executable(Breakout
	main.cpp
	"PlayerEntity.hpp"
	"PlayerEntity.cpp"
	"InputService.hpp"
	"InputService.cpp"
	"CastleEntity.cpp"
	
)

set_property(TARGET Breakout PROPERTY CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-D_GLIBCXX_USE_CXX11_ABI=0 -DNOGDI -DWIN32")

target_link_libraries(Breakout Engine)