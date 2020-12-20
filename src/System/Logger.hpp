#pragma once

#include "Platform.hpp"

enum class LogType
{
    Info,
    Error
};

class Console;

void Log(const char* format, ...);
void Log(LogType type, const char* format, ...);

void InitializeLogging(const char* fileName, Console* console);
void ShutdownLogging();


[[noreturn]]
void FatalError(const char* format, ...) PRINTF_ARGS(1, 2);

static inline void Assert(bool condition, const char* message = nullptr)
{
    if(!condition)
    {
        FatalError("Assert failed: %s", message != nullptr ? message : "");
    }
}

#define ONCE(action_) { static bool alreadyHappened = false; if(!alreadyHappened) { action_; alreadyHappened = true;  }}