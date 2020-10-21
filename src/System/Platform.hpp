#pragma once

#include <chrono>

#ifndef __linux__
#define LINUX_ONLY(_args)
#else
#define LINUX_ONLY(_args) _args
#endif

#define PRINTF_ARGS(_format, _args) LINUX_ONLY(__attribute__ ((format (printf, _format, _args))))

using TimePointType = std::chrono::steady_clock::time_point;