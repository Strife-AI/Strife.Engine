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
    binder.GetEngine()->StartLocalServer(port, StringId(mapName));
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
    _scene = std::make_shared<Scene>(_engine, mapSegment->name, _isServer);

    auto screenSize = (_engine->GetSdlManager() != nullptr ? _engine->GetSdlManager()->WindowSize().AsVectorOfType<float>() : Vector2(0, 0));
    _scene->GetCamera()->SetScreenSize(screenSize);
    _scene->GetCamera()->SetZoom(screenSize.y / (1080.0f / 2));

    _engine->Game()->BuildScene(_scene.get());

    _scene->LoadMapSegment(*mapSegment);


    _scene->SendEvent(SceneLoadedEvent());
}
