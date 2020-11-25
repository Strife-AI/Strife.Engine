#pragma once
#include <functional>
#include <optional>


#include "Memory/BlockAllocator.hpp"
#include "Net/NetworkManager.hpp"
#include "Net/ServerGame.hpp"
#include "Tools/ConsoleCmd.hpp"
#include "Tools/ConsoleVar.hpp"

class Scene;
class IGame;
class SdlManager;
class Input;
class Console;
class PlotManager;
class MetricsManager;
class Renderer;
class SceneManager;
class SoundManager;

struct EngineConfig
{
    EngineConfig& SaveConsoleVarsToFile(const std::string& fileName)
    {
        consoleVarsFile = fileName;
        return *this;
    }

    int blockAllocatorSizeBytes = 32 * 1024 * 1024;
    std::optional<std::string> consoleVarsFile = "vars.cfg";
    std::string initialConsoleCmd;
};

class Engine
{
public:
    ~Engine();

    static Engine* GetInstance() { return &_instance; }
    static Engine* Initialize(const EngineConfig& config);

    Input* GetInput() { return _input; }
    SdlManager* GetSdlManager() { return _sdlManager; }
    Console* GetConsole() { return _console; }
    PlotManager* GetPlotManager() { return _plotManager; }
    MetricsManager* GetMetricsManager() { return _metricsManager; }
    Renderer* GetRenderer() { return _renderer; }
    SceneManager* GetSceneManager() { return _sceneManager; }
    BlockAllocator* GetDefaultBlockAllocator() { return _defaultBlockAllocator; }
    SoundManager* GetSoundManager() { return _soundManager; }
    ServerGame* GetServerGame() { return _serverGame.get(); }
    ClientGame* GetClientGame() { return _clientGame.get(); }

    bool ActiveGame() { return _activeGame; }
    void QuitGame() { _activeGame = false; }

    void SetGame(IGame* game)
    {
        _game = game;
    }

    IGame* Game() { return _game; }

    void RunFrame();

    bool IsPaused() const { return isPaused; }
    void PauseGame();
    void ResumeGame();

    bool isInitialized = false;

    ConsoleVar<int> targetFps = ConsoleVar<int>("targetFps", 60);

    void SetLoadResources(const std::function<void()>& loadResources)
    {
        _loadResources = loadResources;
        ReloadResources();
    }

    void ReloadResources()
    {
        _loadResources();
    }

    void StartLocalServer(int port, StringId mapName);
    void ConnectToServer(const char* address, int port);

private:
    Engine() = default;

    void Render(Scene* scene, float deltaTime, float renderDeltaTime);

    bool isPaused = false;
    static Engine _instance;

    Input* _input = nullptr;
    SdlManager* _sdlManager = nullptr;
    Console* _console = nullptr;
    PlotManager* _plotManager = nullptr;
    MetricsManager* _metricsManager = nullptr;
    Renderer* _renderer = nullptr;
    SceneManager* _sceneManager = nullptr;
    BlockAllocator* _defaultBlockAllocator;
    SoundManager* _soundManager;

    std::unique_ptr<ServerGame> _serverGame;
    std::unique_ptr<ClientGame> _clientGame;

    IGame* _game = nullptr;
    bool _activeGame = true;
    EngineConfig _config;

    std::function<void()> _loadResources;
};

extern ConsoleVar<bool> g_developerMode;