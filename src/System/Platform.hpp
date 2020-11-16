#pragma once

#include <chrono>

#ifndef _linux
#define LINUX_ONLY(_args) _args
using TimePointType = std::chrono::high_resolution_clock::time_point;
#else
using TimePointType = std::chrono::steady_clock::time_point;
#endif

#define PRINTF_ARGS(_format, _args) LINUX_ONLY(__attribute__ ((format (printf, _format, _args))))
