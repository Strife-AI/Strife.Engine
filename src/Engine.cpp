#ifdef __linux__
#include <unistd.h>
#endif

#include <chrono>

#include "Engine.hpp"
#include "Renderer/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Memory/ResourceManager.hpp"
#include <thread>
#include "Scene/IGame.hpp"
#include "System/Input.hpp"
#include "Renderer/SdlManager.hpp"
#include "Tools/Console.hpp"
#include "Tools/PlotManager.hpp"
#include "Tools/MetricsManager.hpp"
#include "Scene/SceneManager.hpp"
#include "Sound/SoundManager.hpp"
#include "UI/UI.hpp"

using namespace std::chrono;

void SleepMicroseconds(unsigned int microseconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

Engine Engine::_instance;

#ifdef RELEASE_BUILD
extern ConsoleVar<bool> g_developerMode("developer-mode", false, false);
#else
extern ConsoleVar<bool> g_developerMode("developer-mode", true, true);
#endif

ConsoleVar<bool> g_stressTest("stress-test", false, false);
int stressCount = 0;

ConsoleVar<bool> isServer("server", false);

Engine* Engine::Initialize(const EngineConfig& config)
{
    InitializeLogging("log.txt");

    Engine* engine = &_instance;

    if (engine->isInitialized)
    {
        FatalError("Engine::Initialize() called more than once");
    }

    engine->_config = config;
    engine->_defaultBlockAllocator = new BlockAllocator(config.blockAllocatorSizeBytes);

    BlockAllocator::SetDefaultAllocator(engine->GetDefaultBlockAllocator());

    // Needs to be initialized first for logging
    engine->_console = new Console;

    // Load variables from vars.cfg
    // This is needed to be done here for loading up configurations for resolution and fullscreen
    if (config.consoleVarsFile.has_value())
    {
        engine->_console->LoadVariables(config.consoleVarsFile.value().c_str());
    }

    engine->_console->Execute(config.initialConsoleCmd);

    Log("==============================================================\n");
    Log("Initializing engine\n");

    engine->isInitialized = true;

    Log("Initializing resource manager\n");
    ResourceManager::Initialize(engine);

    engine->_input = new Input;
    Log("Initializing SDL2\n");
    engine->_sdlManager = new SdlManager(engine->_input);

    engine->_plotManager = new PlotManager;
    engine->_metricsManager = new MetricsManager;

    Log("Initializing renderer\n");
    engine->_renderer = new Renderer;

    Log("Initializing sound\n");
    engine->_soundManager = new SoundManager;

    engine->_sceneManager = new SceneManager(engine);

    engine->_networkManager = new NetworkManager(isServer.Value());

    UiCanvas::Initialize(engine->_soundManager);

    WindowSizeChangedEvent(engine->_sdlManager->WindowSize().x, engine->_sdlManager->WindowSize().y).Send();

    return &_instance;
}

Engine::~Engine()
{
    if (isInitialized)
    {
        Log("Shutting down engine...\n");
        try
        {
            if (_config.consoleVarsFile.has_value())
            {
                _console->SerializeVariables(_config.consoleVarsFile.value().c_str());
            }

            delete _renderer;
            delete _metricsManager;
            delete _plotManager;
            delete _sdlManager;
            delete _input;
            delete _networkManager;
        }
        catch(const std::exception& e)
        {
            Log("Failed to shutdown engine: %s\n", e.what());
        }

        Log("Shutdown complete\n");

        ShutdownLogging();
        delete _console;
        _console = nullptr;

        isInitialized = false;
    }
}

static TimePointType lastFrameStart;

void ThrottleFrameRate()
{
    auto currentFrameStart = high_resolution_clock::now();
    auto deltaTimeMicroseconds = duration_cast<microseconds>(currentFrameStart - lastFrameStart);
    auto realDeltaTime = static_cast<float>(deltaTimeMicroseconds.count()) / 1000000;
    auto desiredDeltaTime = 1.0f / Engine::GetInstance()->targetFps.Value();

    auto diffBias = 0.01f;
    auto diff = desiredDeltaTime - realDeltaTime - diffBias;
    auto desiredEndTime = lastFrameStart + std::chrono::microseconds((int)(desiredDeltaTime * 1000000));

    if(diff > 0)
    {
        SleepMicroseconds(1000000 * diff);
    }

    while(high_resolution_clock::now() < desiredEndTime)
    {

    }

    lastFrameStart = high_resolution_clock::now();
}

ConsoleVar<float> g_timeScale("time-scale", 1.125f);

void Engine::RunFrame()
{
    static int frameCount = 0;
    ++frameCount;

    if (!g_stressTest.Value())
    {
        ThrottleFrameRate();
    }

    _input->Update();

    _sdlManager->Update();

    _sceneManager->DoSceneTransition();

    Scene* scene = _sceneManager->GetScene();

    auto currentFrameStart = high_resolution_clock::now();

    if (scene->isFirstFrame)
    {
        scene->lastFrameStart = currentFrameStart;
        scene->isFirstFrame = false;
    }

    auto deltaTimeMicroseconds = duration_cast<microseconds>(currentFrameStart - scene->lastFrameStart);
    auto realDeltaTime = static_cast<float>(deltaTimeMicroseconds.count()) / 1000000 * g_timeScale.Value();

    float renderDeltaTime = !isPaused
        ? realDeltaTime
        : 0;

    if(g_stressTest.Value())
    {
        realDeltaTime = renderDeltaTime = 0.1;
        targetFps.SetValue(1000);

        if(scene->timeSinceStart > 0.1 * 60 * 60)
        {
            QuitGame();
        }

        //if(_console->IsOpen())
        //{
        //    _console->Close();
        //}
    }

    scene->deltaTime = renderDeltaTime;

    _soundManager->UpdateActiveSoundEmitters(scene->deltaTime);

    Entity* soundListener;
    if (scene->soundListener.TryGetValue(soundListener))
    {
        //_soundManager->SetListenerPosition(soundListener->Center(), soundListener->GetVelocity());
    }

    scene->UpdateEntities(renderDeltaTime);

    if (g_developerMode.Value())
    {
        if (_console->IsOpen())
        {
            _console->HandleInput(_input);
        }

        bool tildePressed = InputButton(SDL_SCANCODE_GRAVE).IsPressed();

        if (tildePressed)
        {
            if (_console->IsOpen())
            {
                _console->Close();
                isPaused = false;
            }
            else
            {
                _console->Open();
                isPaused = true;
            }
        }
    }
    else if (_console->IsOpen())
    {
        _console->Close();
        isPaused = false;
    }

    Render(scene, realDeltaTime, renderDeltaTime);

    if (frameCount % 60 == 0)
    {
        int fps = floor(1.0f / realDeltaTime);
        char windowTitle[128];
        snprintf(windowTitle, sizeof(windowTitle), "C.H.A.S.E.R. - %d fps", fps);
        SDL_SetWindowTitle(_sdlManager->Window(), windowTitle);
    }

    {
        float fps = 1.0f / realDeltaTime;
        _metricsManager->GetOrCreateMetric("fps")->Add(fps);
    }

    _input->GetMouse()->SetMouseScale(Vector2::Unit() * scene->GetCamera()->Zoom());

    scene->lastFrameStart = currentFrameStart;

    if(_sdlManager->ReceivedQuit())
    {
        _activeGame = false;
    }
}

void Engine::Render(Scene* scene, float deltaTime, float renderDeltaTime)
{
    _sdlManager->BeginRender();

    auto screenSize = _sdlManager->WindowSize().AsVectorOfType<float>();
    scene->GetCamera()->SetScreenSize(screenSize);
    scene->GetCamera()->SetZoom(1);// screenSize.y / (1080 / 2));

    auto camera = scene->GetCamera();
    _renderer->BeginRender(camera, Vector2(0, 0), renderDeltaTime, scene->timeSinceStart);
    scene->RenderEntities(_renderer);

    scene->SendEvent(RenderImguiEvent());

    // FIXME: add proper ambient lighting
    _renderer->RenderRectangle(_renderer->GetCamera()->Bounds(), Color(18, 21, 26), 0.99);
    //_renderer->RenderRectangle(_renderer->GetCamera()->Bounds(), Color(255, 255, 255), 0.99);
    _renderer->DoRendering();

    // Render HUD
    _renderer->SetRenderOffset(scene->GetCamera()->TopLeft());
    scene->RenderHud(_renderer);

    if (g_stressTest.Value())
    {
        char text[1024];
        sprintf(text, "Time elapsed: %f hours", scene->timeSinceStart / 60.0f / 60.0f);

        _renderer->RenderString(
            FontSettings(ResourceManager::GetResource<SpriteFont>("console-font"_sid)),
            text,
            Vector2(200, 200),
            -1);
    }

    _renderer->RenderSpriteBatch();

    // Render UI
    {
        Camera uiCamera;
        uiCamera.SetScreenSize(scene->GetCamera()->ScreenSize());
        uiCamera.SetZoom(screenSize.y / 768.0f);
        _renderer->BeginRender(&uiCamera, uiCamera.TopLeft(), renderDeltaTime, scene->timeSinceStart);
        _input->GetMouse()->SetMouseScale(Vector2::Unit() * uiCamera.Zoom());

        scene->BroadcastEvent(RenderUiEvent(_renderer));

        if (_console->IsOpen())
        {
            _console->Render(_renderer, deltaTime);
        }

        _console->Update();

        _renderer->RenderSpriteBatch();
    }

    _plotManager->RenderPlots();

    if (!g_stressTest.Value() || true)
    {
        _sdlManager->EndRender();
    }
}

void Engine::PauseGame()
{
    if (isPaused)
    {
        return;
    }
    else
    {
        isPaused = true;
    }
}

void Engine::ResumeGame()
{
    if (!isPaused)
    {
        return;
    }
    else
    {
        isPaused = false;
    }
}

static void ReloadResources(ConsoleCommandBinder& binder)
{
    binder.Help("Reloads the resource files");

    Engine::GetInstance()->ReloadResources();
}

ConsoleCmd _reloadCmd("reload", ReloadResources);