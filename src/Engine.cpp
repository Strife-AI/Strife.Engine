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


Engine Engine::_instance;

#ifdef RELEASE_BUILD
extern ConsoleVar<bool> g_developerMode("developer-mode", false, false);
#else
extern ConsoleVar<bool> g_developerMode("developer-mode", true, true);
#endif

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

    engine->_sceneManager = new SceneManager(engine, false);

    if(isServer.Value())
    {
        engine->targetFps.SetValue(30);
    }
    else
    {
        engine->targetFps.SetValue(60);
    }

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

void Engine::RunFrame()
{
    if (_serverGame != nullptr)
    {
        _serverGame->RunFrame();
    }

    if(_clientGame != nullptr)
    {
        _clientGame->RunFrame();
    }
}

void Engine::PauseGame()
{
    isPaused = true;
}

void Engine::ResumeGame()
{
    isPaused = false;
}

void Engine::StartLocalServer(int port, StringId mapName)
{
    SLNet::SocketDescriptor sd;
    auto peerInterface = SLNet::RakPeerInterface::GetInstance();
    auto result = peerInterface->Startup(1, &sd, 1);

    if (result != SLNet::RAKNET_STARTED)
    {
        FatalError("Failed to startup client");
    }

    _serverGame = std::make_unique<ServerGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("127.0.0.1", 1)));
    _clientGame = std::make_unique<ClientGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("127.0.0.1", 2)));

    _serverGame->networkInterface.SetLocalAddress(&_clientGame->networkInterface, _clientGame->localAddress);
    _clientGame->networkInterface.SetLocalAddress(&_serverGame->networkInterface, _serverGame->localAddress);

    _clientGame->sceneManager.TrySwitchScene(mapName);
    _serverGame->sceneManager.TrySwitchScene(mapName);
}

static void ReloadResources(ConsoleCommandBinder& binder)
{
    binder.Help("Reloads the resource files");

    Engine::GetInstance()->ReloadResources();
}

ConsoleCmd _reloadCmd("reload", ReloadResources);