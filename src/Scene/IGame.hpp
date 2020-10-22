#pragma once
#include <memory>
#include <optional>

#include "Memory/StringId.hpp"

struct EngineConfig;
class Scene;
class Engine;
class SceneManager;

struct GameConfig
{
    GameConfig& SetDefaultScene(StringId sceneName)
    {
        defaultScene = sceneName;
        return *this;
    }

    GameConfig& SetGameName(const std::string& gameName_)
    {
        gameName = gameName_;
        return *this;
    }

    GameConfig& SetWindowCaption(const std::string& windowCaption_)
    {
        windowCaption = windowCaption_;
        return *this;
    }

    GameConfig& ExecuteUserConfig(const std::string& fileName)
    {
        userConfigFile = fileName;
        return *this;
    }

    StringId defaultScene = "empty-map"_sid;
    std::string gameName;
    std::string windowCaption;
    std::optional<std::string> userConfigFile;
};

class IGame
{
public:
    virtual ~IGame() = default;

    /// <summary>
    /// Callback for configuring the game. This is where game services should be added.
    /// </summary>
    /// <param name="builder"></param>
    virtual void ConfigureGame(GameConfig& config) = 0;

    /// <summary>
    /// Callback for doing scene-specific setup e.g. for the main menu. This is where scene services should be
    /// added.
    /// </summary>
    /// <param name="scene"></param>
    /// <param name="name"></param>
    virtual void BuildScene(Scene* scene) { }

    virtual void ConfigureEngine(EngineConfig& config) { }

    Engine* GetEngine() const { return _engine; }
    void Run();

private:
    Engine* _engine = nullptr;
    GameConfig _config;
};