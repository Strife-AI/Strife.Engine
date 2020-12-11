#ifdef __linux__
#include <unistd.h>
#endif

#include <chrono>

#include "Engine.hpp"
#include "Renderer/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Memory/ResourceManager.hpp"
#include <thread>

#include "../Strife.ML/NewStuff.hpp"
#include "ML/ML.hpp"
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

ConsoleVar<bool> g_isServer("server", false);

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

    Log("Running as %s\n", g_isServer.Value() ? "server" : "client");

    engine->isInitialized = true;

    Log("Initializing resource manager\n");
    ResourceManager::Initialize(engine);

    engine->_input = new Input;

    if(!g_isServer.Value())
    {
        Log("Initializing SDL2\n");
        engine->_sdlManager = new SdlManager(engine->_input, false);
    }

    engine->_plotManager = new PlotManager;
    engine->_metricsManager = new MetricsManager;

    if(!g_isServer.Value())
    {
        Log("Initializing renderer\n");
        engine->_renderer = new Renderer;
        WindowSizeChangedEvent(engine->_sdlManager->WindowSize().x, engine->_sdlManager->WindowSize().y).Send();
    }

    Log("Initializing sound\n");
    engine->_soundManager = new SoundManager(g_isServer.Value());

    engine->_neuralNetworkManager = std::make_unique<NeuralNetworkManager>();

    UiCanvas::Initialize(engine->_soundManager);

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

            _serverGame = nullptr;
            _clientGame = nullptr;
            _neuralNetworkManager = nullptr;

            auto peerInterface = SLNet::RakPeerInterface::GetInstance();
            if(peerInterface->IsActive())
            {
                peerInterface->Shutdown(500);
            }
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

void SleepMicroseconds(unsigned int microseconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

static float GetTimeSeconds()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto now = std::chrono::high_resolution_clock::now();
    auto deltaTimeMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
    return static_cast<float>(deltaTimeMicroseconds.count()) / 1000000;
}

void AccurateSleepFor(float seconds)
{
    auto bias = 0;//0.01f;
    auto diff = seconds - bias;
    auto desiredEndTime = std::chrono::high_resolution_clock::now() + std::chrono::microseconds((int)(seconds * 1000000));

    if (diff > 0)
    {
        SleepMicroseconds(1000000 * diff);
    }

    while (std::chrono::high_resolution_clock::now() < desiredEndTime)
    {
        // No-op
    }
}

void Engine::RunFrame()
{
    std::shared_ptr<BaseGameInstance> games[2];
    int totalGames = 0;

    if (_serverGame != nullptr)
    {
        games[totalGames++] = _serverGame;
    }

    if(_clientGame != nullptr)
    {
        games[totalGames++] = _clientGame;
    }

    if (totalGames == 0)
    {
        return;
    }

    BaseGameInstance* nextGameToRun = games[0].get();

    for(int i = 1; i < totalGames; ++i)
    {
        if(games[i]->nextUpdateTime < nextGameToRun->nextUpdateTime)
        {
            nextGameToRun = games[i].get();
        }
    }

    float now = GetTimeSeconds();
    float timeUntilUpdate = nextGameToRun->nextUpdateTime - now;

    AccurateSleepFor(timeUntilUpdate);
    nextGameToRun->RunFrame(GetTimeSeconds());
    nextGameToRun->nextUpdateTime = nextGameToRun->nextUpdateTime + 1.0f / nextGameToRun->targetTickRate;
}

void Engine::PauseGame()
{
    isPaused = true;
}

void Engine::ResumeGame()
{
    isPaused = false;
}

void ConnectCommand(ConsoleCommandBinder& binder)
{
    std::string address;
    int port;

    binder
        .Bind(address, "serverAddress")
        .Bind(port, "port")
        .Help("Connects to a server");

    Engine::GetInstance()->ConnectToServer(address.c_str(), port);
}

ConsoleCmd connectCmd("connect", ConnectCommand);

void Engine::StartServer(int port, StringId mapName)
{
    SLNet::SocketDescriptor sd(port, nullptr);
    auto peerInterface = SLNet::RakPeerInterface::GetInstance();

    if(peerInterface->IsActive())
    {
        peerInterface->Shutdown(500);
    }

    auto result = peerInterface->Startup(1, &sd, 1);

    if (result != SLNet::RAKNET_STARTED)
    {
        FatalError("Failed to startup server");
    }

    peerInterface->SetMaximumIncomingConnections(ServerGame::MaxClients);

    Log("Listening on %d...\n", port);

    _serverGame = std::make_shared<ServerGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("127.0.0.2", 1)));
    _clientGame = nullptr;

    _serverGame->sceneManager.TrySwitchScene(mapName);
}

void Engine::ConnectToServer(const char* address, int port)
{
    SLNet::SocketDescriptor sd;
    auto peerInterface = SLNet::RakPeerInterface::GetInstance();

    if (peerInterface->IsActive())
    {
        peerInterface->Shutdown(500);
    }

    auto result = peerInterface->Startup(1, &sd, 1);

    Log("Trying to connect to %s:%d...\n", address, port);

    if (result != SLNet::RAKNET_STARTED)
    {
        FatalError("Failed to startup client");
    }

    auto connectResult = peerInterface->Connect(address, port, nullptr, 0);

    if (connectResult != SLNet::CONNECTION_ATTEMPT_STARTED)
    {
        Log("Failed to initiate server connection");
    }

    _serverGame = nullptr;
    _clientGame = std::make_shared<ClientGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("127.0.0.2", 1)));
}

void Engine::StartLocalServer(int port, StringId mapName)
{
    if(g_isServer.Value())
    {
        Log(LogType::Error, "Can't start local server when running in server mode\n");
        return;
    }

    StartServer(port, mapName);

    auto peerInterface = SLNet::RakPeerInterface::GetInstance();

    _clientGame = std::make_shared<ClientGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("127.0.0.3", 1)));

    _serverGame->networkInterface.SetLocalAddress(&_clientGame->networkInterface, _clientGame->localAddress);
    _clientGame->networkInterface.SetLocalAddress(&_serverGame->networkInterface, _serverGame->localAddress);

    _clientGame->sceneManager.TrySwitchScene(mapName);

    unsigned char data[1] = { (unsigned char)PacketType::NewConnection };

    _clientGame->networkInterface.SendReliable(_serverGame->localAddress, data);
}

static void ReloadResources(ConsoleCommandBinder& binder)
{
    binder.Help("Reloads the resource files");

    Engine::GetInstance()->ReloadResources();
}

ConsoleCmd _reloadCmd("reload", ReloadResources);