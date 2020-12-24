#pragma once

#include <SDL2/SDL.h>
#include <Math/Vector2.hpp>

#include "Tools/ConsoleVar.hpp"

#include "Memory/EngineEvent.hpp"

class Input;
class Logger;

extern ConsoleVar<Vector2> g_Resolution;
extern ConsoleVar<bool> g_FullscreenOnStart;
extern ConsoleVar<bool> g_EnableVsync;

struct WindowSizeChangedEvent : EngineEvent<WindowSizeChangedEvent>
{
    WindowSizeChangedEvent(int w, int h)
        : width(w),
        height(h)
    {
        
    }

    int width;
    int height;
};

class SdlManager
{
public:
    SdlManager(Input* input, bool isHeadless);

    ~SdlManager();

    SDL_Window* Window() const { return _window; }
    Vector2i WindowSize() const;
    float DpiRatio() const { return _dpiRatio; }
    bool ReceivedQuit() const { return _receivedQuit; }

    void SetScreenSize(int w, int h);

    void SetWindowCaption(const char* title);

    void SetFullscreen(bool isFullscreen);
    void SetVsync(bool isEnabled);

    void HideCursor();
    void DisplayCursor();

    void Update();
    void BeginRender();
    void EndRender();

private:
    void SetupOpenGl(bool isHeadless);

    Logger* _logger;

    Input* _input;
    SDL_GameController* _controller = nullptr;

    SDL_Window* _window;
    SDL_GLContext _context;
    float _dpiRatio;

    bool _receivedQuit = false;
    bool _cursorVisible = true;
};
