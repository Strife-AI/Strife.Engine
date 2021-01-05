#ifdef __linux__
#include <unistd.h>
#endif

#include <chrono>

#include "Engine.hpp"
#include "Renderer/Renderer.hpp"
#include <thread>
#include "slikenet/PacketConsoleLogger.h"

#include "../Strife.ML/NewStuff.hpp"
#include "ML/ML.hpp"
#include "Scene/IGame.hpp"
#include "System/Input.hpp"
#include "Renderer/SdlManager.hpp"
#include "Tools/Console.hpp"
#include "Tools/PlotManager.hpp"
#include "Tools/MetricsManager.hpp"
#include "Sound/SoundManager.hpp"
#include "UI/UI.hpp"

using namespace std::chrono;

#ifdef RELEASE_BUILD
extern ConsoleVar<bool> g_developerMode("developer-mode", false, false);
#else
extern ConsoleVar<bool> g_developerMode("developer-mode", true, true);
#endif

ConsoleVar<bool> g_isServer("server", false);

Engine::Engine(const EngineConfig& config)
{
    _config = config;
    _defaultBlockAllocator = std::make_unique<BlockAllocator>(config.blockAllocatorSizeBytes);

    BlockAllocator::SetDefaultAllocator(GetDefaultBlockAllocator());

    // Needs to be initialized first for logging
    _console = std::make_unique<Console>(this);
    InitializeLogging("log.txt", _console.get());

    // Load variables from vars.cfg
    // This is needed to be done here for loading up configurations for resolution and fullscreen
    if (config.consoleVarsFile.has_value())
    {
        _console->LoadVariables(config.consoleVarsFile.value().c_str());
    }

    _console->Execute(config.initialConsoleCmd);

    Log("==============================================================\n");
    Log("Initializing engine\n");

    Log("Running as %s\n", g_isServer.Value() ? "server" : "client");

    _input = Input::GetInstance();

    if(!g_isServer.Value())
    {
        Log("Initializing SDL2\n");
        _sdlManager = std::make_unique<SdlManager>(_input, false);
        _plotManager = std::make_unique<PlotManager>(_sdlManager.get());
    }

    _metricsManager = std::make_unique<MetricsManager>();

    if(!g_isServer.Value())
    {
        Log("Initializing renderer\n");
        _renderer = std::make_unique<Renderer>();
        WindowSizeChangedEvent(_sdlManager->WindowSize().x, _sdlManager->WindowSize().y).Send();
    }

    Log("Initializing sound\n");
    _soundManager = std::make_unique<SoundManager>(g_isServer.Value());

    _neuralNetworkManager = std::make_unique<NeuralNetworkManager>();

    UiCanvas::Initialize(_soundManager.get());
}

Engine::~Engine()
{
    Log("Shutting down engine...\n");
    try
    {
        if (_config.consoleVarsFile.has_value())
        {
            _console->SerializeVariables(_config.consoleVarsFile.value().c_str());
        }

        if(_clientGame != nullptr)
        {
            _clientGame->Disconnect();
        }

        auto peerInterface = SLNet::RakPeerInterface::GetInstance();
        peerInterface->Shutdown(2000, 0, HIGH_PRIORITY);
    }
    catch(const std::exception& e)
    {
        Log("Failed to shutdown engine: %s\n", e.what());
    }

    Log("Shutdown complete\n");

    ShutdownLogging();
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
    std::string address = "142.93.49.216";
    int port = 60001;

    binder.TryBind(address, "serverAddress");
    binder.TryBind(port, "port");
    binder.Help("Connects to a server");

    binder.GetEngine()->ConnectToServer(address.c_str(), port);
}

ConsoleCmd connectCmd("connect", ConnectCommand);

void Engine::StartServer(int port, const char* mapName)
{
    SLNet::SocketDescriptor sd(port, nullptr);
    auto peerInterface = SLNet::RakPeerInterface::GetInstance();

    if(peerInterface->IsActive())
    {
        peerInterface->Shutdown(500);
    }

    auto result = peerInterface->Startup(ServerGame::MaxClients, &sd, 1);

    if (result != SLNet::RAKNET_STARTED)
    {
        FatalError("Failed to startup server");
    }

    peerInterface->SetMaximumIncomingConnections(ServerGame::MaxClients);

    Log("Listening on %d...\n", port);

    _serverGame = std::make_shared<ServerGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("0.0.0.2", 1)));
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
    _clientGame = std::make_shared<ClientGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("0.0.0.2", 1)));
    _clientGame->serverAddress = SLNet::SystemAddress(address, port);
}

SLNet::PacketLogger logger;

void Engine::StartLocalServer(int port, const char* mapName)
{
    if(g_isServer.Value())
    {
        Log(LogType::Error, "Can't start local server when running in server mode\n");
        return;
    }

    StartServer(port, mapName);

    auto peerInterface = SLNet::RakPeerInterface::GetInstance();

    peerInterface->AttachPlugin(&logger);

    _clientGame = std::make_shared<ClientGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("0.0.0.3", 1)));

    _serverGame->networkInterface.SetLocalAddress(&_clientGame->networkInterface, _clientGame->localAddress);
    _clientGame->networkInterface.SetLocalAddress(&_serverGame->networkInterface, _serverGame->localAddress);

    _clientGame->sceneManager.TrySwitchScene(mapName);

    unsigned char data[1] = { (unsigned char)PacketType::NewConnection };

    _clientGame->networkInterface.SendReliable(_serverGame->localAddress, data);
}

void Engine::StartSinglePlayerGame(const char* mapName)
{
    auto peerInterface = SLNet::RakPeerInterface::GetInstance();
    _clientGame = std::make_shared<ClientGame>(this, peerInterface, SLNet::AddressOrGUID(SLNet::SystemAddress("0.0.0.3", 1)));
    _clientGame->sceneManager.TrySwitchScene(mapName);
}

static void ReloadResources(ConsoleCommandBinder& binder)
{
    binder.Help("Reloads the resource files");

    binder.GetEngine()->ReloadResources();
}

ConsoleCmd _reloadCmd("reload", ReloadResources);