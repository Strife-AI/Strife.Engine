#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <filesystem>


#include "Memory/StringId.hpp"
#include "Scene.hpp"

struct EngineConfig;
class Scene;
class Engine;
class SceneManager;
class Project;
class ResourceManager;

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

    GameConfig& EnableDevConsole(const std::string& consoleFont)
    {
        devConsoleFont = consoleFont;
        return *this;
    }

    GameConfig& AddResourcePack(const std::string& resourcePackFileName)
    {
        resourcePacks.push_back(resourcePackFileName);
        return *this;
    }

    GameConfig& SetProjectFile(const std::filesystem::path& path)
    {
        projectFilePath = path;
        return *this;
    }

    GameConfig& EnableMultiplayer()
    {
        isMultiplayer = true;
        return *this;
    }

    GameConfig& SetPerspective(ScenePerspective perspective)
    {
        this->perspective = perspective;
        return *this;
    }

    StringId defaultScene = "empty-map"_sid;
    std::string gameName;
    std::filesystem::path projectFilePath;
    std::optional<std::string> windowCaption;
    std::optional<std::string> userConfigFile;
    std::optional<std::string> devConsoleFont;
    std::vector<std::string> resourcePacks;
    bool isMultiplayer = false;
    ScenePerspective perspective = ScenePerspective::Orothgraphic;
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

    virtual void OnGameStart() { }

    virtual void LoadResources(ResourceManager* resourceManager) { }

    virtual void Render(Renderer* renderer) { }

    Engine* GetEngine() { return _engine.get(); }
    void Run();

    const GameConfig& GetConfig() const
    {
        return _config;
    }

    std::shared_ptr<Project> project;
private:
    std::unique_ptr<Engine> _engine = nullptr;
    GameConfig _config;
};