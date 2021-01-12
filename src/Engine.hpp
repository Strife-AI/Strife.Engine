#pragma once
#include <functional>
#include <optional>
#include <Project/Project.hpp>


#include "Memory/BlockAllocator.hpp"
#include "Net/ServerGame.hpp"
#include "Tools/ConsoleCmd.hpp"
#include "Tools/ConsoleVar.hpp"

struct NeuralNetworkManager;

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
    explicit Engine(const EngineConfig& config);
    ~Engine();

    Input* GetInput() { return _input; }
    SdlManager* GetSdlManager() { return _sdlManager.get(); }
    Console* GetConsole() { return _console.get(); }
    PlotManager* GetPlotManager() { return _plotManager.get(); }
    MetricsManager* GetMetricsManager() { return _metricsManager.get(); }
    Renderer* GetRenderer() { return _renderer.get(); }
    BlockAllocator* GetDefaultBlockAllocator() { return _defaultBlockAllocator.get(); }
    SoundManager* GetSoundManager() { return _soundManager.get(); }
    ServerGame* GetServerGame() { return _serverGame.get(); }
    ClientGame* GetClientGame() { return _clientGame.get(); }
    NeuralNetworkManager* GetNeuralNetworkManager() { return _neuralNetworkManager.get();  }

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

    void SetLoadResources(const std::function<void()>& loadResources)
    {
        _loadResources = loadResources;
        ReloadResources();
    }

    void ReloadResources()
    {
        _loadResources();
    }

    void StartServer(int port, const char* mapName);
    void ConnectToServer(const char* address, int port);
    void StartLocalServer(int port, const char* mapName);
    void StartSinglePlayerGame(const char* mapName);

private:
    Engine() = default;

    bool isPaused = false;

    Input* _input;
    std::unique_ptr<SdlManager> _sdlManager;
    std::unique_ptr<Console> _console;
    std::unique_ptr<PlotManager> _plotManager;
    std::unique_ptr<MetricsManager> _metricsManager;
    std::unique_ptr<Renderer> _renderer;
    std::unique_ptr<BlockAllocator> _defaultBlockAllocator;
    std::unique_ptr<SoundManager> _soundManager;
    std::unique_ptr<NeuralNetworkManager> _neuralNetworkManager;

    std::shared_ptr<ServerGame> _serverGame;
    std::shared_ptr<ClientGame> _clientGame;

    IGame* _game = nullptr;
    bool _activeGame = true;
    EngineConfig _config;

    Project _project;

    std::function<void()> _loadResources;
};

extern ConsoleVar<bool> g_developerMode;