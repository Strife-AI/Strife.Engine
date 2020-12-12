#include <stdexcept>
#include <cstdarg>
#include <Engine.hpp>
#include <chrono>
#include <ctime>  

#include "Logger.hpp"


#include <fstream>
#include <thread>
#include <iostream>

#include "SDL.h"
#include "Container/ConcurrentQueue.hpp"
#include "Tools/Console.hpp"

// TODO: make this thread safe
void FatalError(const char* format, ...)
{
    va_list list;
    va_start(list, format);

    char str[8192];
    vsnprintf(str, sizeof(str), format, list);

    Log(LogType::Error, "FATAL ERROR: %s\n", str);

#if true
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        "Fatal Error",
        str,
        nullptr);

#endif

    // TODO
    throw std::runtime_error(str);
}

struct LogEvent
{
    LogEvent(std::string message_, std::time_t time_, LogType type_)
        : message(message_),
        time(time_),
        type(type_)
    {
        
    }

    std::string message;
    std::time_t time;
    LogType type;
};

static std::unique_ptr<std::thread> g_loggingThread;
static ConcurrentQueue<LogEvent> g_logMessages;
static std::ofstream g_logFile;
static bool g_finishLogging = false;
static bool g_logToFile = true;
static bool g_logToConsole = false;
static Console* g_console;

extern ConsoleVar<bool> g_isServer;

void LoggingThread()
{
    do
    {
        while (!g_logMessages.IsEmpty())
        {
            auto event = g_logMessages.Front();

            std::string timeString(std::ctime(&event.time));

            while(timeString.size() > 0 && (timeString[timeString.size() - 1] == '\n'))
            {
                timeString.pop_back();
            }

            auto message = (event.type == LogType::Info ? "[INFO " : "[ERR  ") + timeString + "] " + event.message;

            g_logMessages.Dequeue();

            g_console->logItems.Enqueue(message);

            if (g_logFile)
            {
                g_logFile << message;
                g_logFile.flush();
            }

            if (g_isServer.Value())
            {
                std::cout << message;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (!g_finishLogging || !g_logMessages.IsEmpty());
}

void InitializeLogging(const char* fileName, Console* console)
{
    g_console = console;
    g_logFile.open(fileName, std::ofstream::app);
    g_logToFile = g_logFile.is_open();
    g_finishLogging = false;
    g_loggingThread = std::make_unique<std::thread>(LoggingThread);
    g_logToConsole = true;
}

void ShutdownLogging()
{
    g_finishLogging = true;

    if (g_loggingThread != nullptr)
    {
        if (g_loggingThread->joinable())
        {
            g_loggingThread->join();
        }

        g_loggingThread = nullptr;
    }

    g_logFile.close();
}

void Log(const char* format, ...)
{
    va_list list;
    va_start(list, format);

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, list);
    g_logMessages.Enqueue({
        buffer,
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
        LogType::Info
    });
}

void Log(LogType type, const char* format, ...)
{
    va_list list;
    va_start(list, format);

    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, list);
    g_logMessages.Enqueue({
        buffer,
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
        type
        });
}
