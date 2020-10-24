#include <iostream>

#include "SdlManager.hpp"


#include "Engine.hpp"
#include "System/Logger.hpp"
#include "System/Input.hpp"
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_gamecontroller.h"

#include "GL/gl3w.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "Tools/Console.hpp"

#if _WIN32
#include "Winuser.h"
#endif

ConsoleVar<Vector2> g_Resolution("pResolution", Vector2(1024, 728), true);
ConsoleVar<bool> g_FullscreenOnStart("pFullscreen", true, true);
ConsoleVar<bool> g_EnableVsync("pVsync", true, true);

static constexpr int OpenGlMajorVersion = 3;
static constexpr int OpenGlMinorVersion = 3;

void ResolutionCommand(ConsoleCommandBinder& binder)
{
    int w, h;
    binder
        .Bind(w, "width")
        .Bind(h, "height")
        .Help("Changes the screen resolution");

    Engine::GetInstance()->GetSdlManager()->SetScreenSize(w, h);
    g_Resolution.SetValue(Vector2(w, h));
}
ConsoleCmd g_resolutionCmd("resolution", ResolutionCommand);

#include <Windows.h>
#include <ShellScalingAPI.h>
#include <comdef.h>

#pragma comment(lib, "Shcore.lib")

ConsoleVar<bool> g_firstEverRun("first-time", true, true);

std::vector<std::pair<std::string, Vector2>> resolutions
{
    {"2560 X 1440", Vector2(2560, 1440)},
    {"1680 X 1850", Vector2(1680, 1850)},
    {"1920 X 1080", Vector2(1920, 1080)},
    {"1440 X 900", Vector2(1440, 900)},
    {"1024 X 768", Vector2(1024, 768)},
    {"1366 X 768", Vector2(1366, 768)},
    {"1360 X 768", Vector2(1360, 768)},
};

void SetDefaultValuesOnFirstRun()
{
    if (!g_firstEverRun.Value())
    {
        return;
    }

    g_firstEverRun.SetValue(false);

    Vector2 closestResolution = resolutions[0].second;

    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0)
    {
        Log("Warning: failed to query default resolution; defaulting to 1920x1080\n");
        closestResolution = Vector2(1920, 1080);
    }
    else
    {
        Vector2 target = Vector2(dm.w, dm.h);

        for (int i = 1; i < resolutions.size(); ++i)
        {
            if ((resolutions[i].second - target).Length() < (closestResolution - target).Length())
            {
                closestResolution = resolutions[i].second;
            }
        }

        Log("Picked %fx%f as initial resolution\n", closestResolution.x, closestResolution.y);
    }

    g_Resolution.SetValue(closestResolution);
}

SdlManager::SdlManager(Input* input)
    : _input(input)
{
#ifdef _WIN32
    HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    if (FAILED(hr))
    {
        _com_error err(hr);
        fwprintf(stderr, L"SetProcessDpiAwareness: %hs\n", err.ErrorMessage());
    }
#endif

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        FatalError("Failed to initialize SDL");
    }

    SetDefaultValuesOnFirstRun();

    SetupOpenGl();

    IMG_Init(IMG_INIT_PNG);

    if (SDL_NumJoysticks() > 0)
    {
        _controller = SDL_GameControllerOpen(0);
    }
}

void SdlManager::SetupOpenGl()
{
#if true
    #if __APPLE__
    // GL 3.2 Core + GLSL 150
        const char* glsl_version = "#version 150";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
// GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OpenGlMajorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OpenGlMinorVersion);
#endif

    // Create window with graphics context
    if(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) != 0)
    {
        Log("Failed to set SDL_GL_DOUBLEBUFFER\n");
    }

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    if(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1) != 0)
    {
        Log("Failed to set stencil buffer bits\n");
    }

#if __APPLE__
    float defaultDpi = 72;
#else
    float defaultDpi = 96;
#endif

    float dpi;
    if (SDL_GetDisplayDPI(0, NULL, &dpi, NULL) != 0) {
        Log("Failed to get DPI; assuming default of %f\n", defaultDpi);
        dpi = defaultDpi;
    }

    _dpiRatio = dpi / defaultDpi;

    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL
        | SDL_WINDOW_INPUT_FOCUS
        | ((g_FullscreenOnStart.Value()) ? SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS : 0));

    _window = SDL_CreateWindow(
        "C.H.A.S.E.R.",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        g_Resolution.Value().x,
        g_Resolution.Value().y,
        window_flags);

    if (_window == nullptr) {
        FatalError("Failed to create window");
    }

    _context = SDL_GL_CreateContext(_window);

    _windowSurface = SDL_GetWindowSurface(_window);
    SDL_GL_MakeCurrent(_window, _context);

    gl3wInit();

    if (!gl3wIsSupported(OpenGlMajorVersion, OpenGlMinorVersion))
    {
        FatalError("OpenGL 3.3 is required to run this game.");
    }

    glEnable(GL_BLEND);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.FontGlobalScale = _dpiRatio;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(_window, _context);
    ImGui_ImplOpenGL3_Init(glsl_version);
#endif

    if (g_EnableVsync.Value()) EnableVsync();
    else DisableVsync();

    int stencilBufferBits;
    if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencilBufferBits) == 0)
    {
        Log("Stencil buffer bits: %d\n", stencilBufferBits);
    }
    else
    {
        Log("Failed to retrieve number of stencil buffer bits\n");
    }

    int hasDoubleBuffer;
    if (SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &hasDoubleBuffer) != 0)
    {
        Log("Failed to query SDL_GL_DOUBLEBUFFER: %s\n", SDL_GetError());
    }
    else
    {
        Log("Has double buffering: %d\n", hasDoubleBuffer);
    }
}

void SdlManager::BeginRender()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(_window);
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    // TODO: don't call this in every frame
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
}

void SdlManager::EndRender()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(_window);
    //glFinish();
}

SdlManager::~SdlManager()
{
    if (_controller != nullptr)
    {
        SDL_GameControllerClose(_controller);
    }

    IMG_Quit();
    SDL_GL_DeleteContext(_context);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void SdlManager::SetScreenSize(int w, int h)
{
    SDL_SetWindowSize(_window, w, h);
    WindowSizeChangedEvent(w, h).Send();

    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void SdlManager::Update()
{
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0)
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_KEYDOWN:
            _input->SendKeyDown(event.key.keysym.scancode);
            break;

        case SDL_KEYUP:
            _input->SendKeyUp(event.key.keysym.scancode);
            break;

        case SDL_CONTROLLERBUTTONDOWN:
            _input->SendButtonDown(event.cbutton.button);
            break;

        case SDL_CONTROLLERBUTTONUP:
            _input->SendButtonUp(event.cbutton.button);
            break;

        case SDL_FINGERDOWN:
            _input->SendTap();
            break;

        case SDL_CONTROLLERDEVICEADDED:
            _controller = SDL_GameControllerOpen(0);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            SDL_GameControllerClose(_controller);
            _controller = nullptr;
            break;

        case SDL_CONTROLLERAXISMOTION:
            _input->SendAxisValue(event.caxis.axis, event.caxis.value);
            break;

        case SDL_QUIT:
            _receivedQuit = true;
            break;

        case SDL_MOUSEWHEEL:
            _input->SendMouseWheelY(event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                ? -event.wheel.y
                : event.wheel.y);
            break;
        }
    }
}

Vector2i SdlManager::WindowSize() const
{
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);

    return Vector2i(w, h);
}

void SdlManager::EnableFullscreen()
{
    if (g_FullscreenOnStart.Value())
    {
        return;
    }

    g_FullscreenOnStart.SetValue(true);
    SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS);

    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void SdlManager::DisableFullscreen()
{
    if (!g_FullscreenOnStart.Value())
    {
        return;
    }

    g_FullscreenOnStart.SetValue(false);
    SDL_SetWindowFullscreen(_window, 0);

    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

void SdlManager::EnableVsync()
{
    g_EnableVsync.SetValue(true);
    SDL_GL_SetSwapInterval(true);
}

void SdlManager::DisableVsync()
{
    // TODO
    g_EnableVsync.SetValue(false);
    SDL_GL_SetSwapInterval(false);
}


void SdlManager::HideCursor()
{
#if _WIN32
    if (_cursorVisible == false)
    {
        return;
    }
    else
    {
        _cursorVisible = false;
        ShowCursor(false);
    }
#endif
}

void SdlManager::DisplayCursor()
{
#if _WIN32
    if (_cursorVisible == true)
    {
        return;
    }
    else
    {
        _cursorVisible = true;
        ShowCursor(true);
    }
#endif
}
