#include <chrono>
#include "Tools/Console.hpp"
#include "Engine.hpp"

#include "SceneManager.hpp"

#include "IGame.hpp"
#include "Scene.hpp"
#include "SdlManager.hpp"
#include "IEntityEvent.hpp"

void MapCmd(ConsoleCommandBinder& binder)
{
    std::string mapName;
    int port = 6666;

    binder
        .Bind(mapName, "map-name");

    binder.TryBind(port, "port", true);

    binder.Help("Change scene to specified map.");

    // TODO: map command for server mode
    Engine::GetInstance()->StartLocalServer(port, StringId(mapName));
}
ConsoleCmd mapCmd("map", MapCmd);

SceneManager::SceneManager(Engine* engine, bool isServer)
    : _engine(engine),
    _scene(new Scene(engine, ""_sid, isServer)),
    _isServer(isServer)
{
    auto emptyMapSegment = new MapSegment;
    emptyMapSegment->name = "empty-map"_sid;
    ResourceManager::AddResource("empty-map"_sid, emptyMapSegment);
}

void SceneManager::DoSceneTransition()
{
    if (_newScene != nullptr)
    {
        _scene->SendEvent(SceneDestroyedEvent());
        DestroyScene();

        _scene = _newScene;
        _newScene = nullptr;

        _scene->SendEvent(SceneLoadedEvent());
    }
}

bool SceneManager::TrySwitchScene(StringId name)
{
    Resource<MapSegment> mapSegment;
    if (ResourceManager::TryGetResource(name, mapSegment))
    {
        BuildNewScene(mapSegment.Value());
        return true;
    }
    else
    {
        Log("Failed to load map %s\n", name.ToString());
        return false;
    }
}

void SceneManager::BuildNewScene(const MapSegment* mapSegment)
{
    delete _newScene;

    _newScene = new Scene(_engine, mapSegment->name, _isServer);

    auto screenSize = (_engine->GetSdlManager() != nullptr ? _engine->GetSdlManager()->WindowSize().AsVectorOfType<float>() : Vector2(0, 0));
    _newScene->GetCamera()->SetScreenSize(screenSize);
    _newScene->GetCamera()->SetZoom(screenSize.y / (1080.0f / 2));

    _engine->Game()->BuildScene(_newScene);
    _newScene->LoadMapSegment(*mapSegment);


    int levelId;
    if (mapSegment->properties.TryGetProperty("levelId", levelId))
    {
        _newScene->levelId = levelId;
    }
}

void SceneManager::DestroyScene()
{
    if(_scene != nullptr)
    {
        delete _scene;
        _scene = nullptr;
    }
}
