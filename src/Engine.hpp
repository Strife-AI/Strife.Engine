#pragma once
#include <functional>

#include "Memory/BlockAllocator.hpp"
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

class Engine
{
public:
    Engine();
    ~Engine();

    static Engine* GetInstance() { return &_instance; }
    static Engine* Initialize();

    Input* GetInput() { return _input; }
    SdlManager* GetSdlManager() { return _sdlManager; }
    Console* GetConsole() { return _console; }
    PlotManager* GetPlotManager() { return _plotManager; }
    MetricsManager* GetMetricsManager() { return _metricsManager; }
    Renderer* GetRenderer() { return _renderer; }
    SceneManager* GetSceneManager() { return _sceneManager; }
    BlockAllocator* GetDefaultBlockAllocator() { return &_defaultBlockAllocator; }
    SoundManager* GetSoundManager() { return _soundManager; }

    bool ActiveGame() { return _activeGame; }
    void QuitGame() { _activeGame = false; }

    void SetGame(IGame* game, SceneManager* sceneManager)
    {
        _game = game;
        _sceneManager = sceneManager;
    }

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

private:
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
    BlockAllocator _defaultBlockAllocator;
    SoundManager* _soundManager;

    IGame* _game = nullptr;
    bool _activeGame = true;

    std::function<void()> _loadResources;
};

extern ConsoleVar<bool> g_developerMode;