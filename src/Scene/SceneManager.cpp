#include <Resource/TilemapResource.hpp>
#include <Net/PlayerCommandRunner.hpp>
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
    binder.GetEngine()->StartLocalServer(port, mapName.c_str());
}
ConsoleCmd mapCmd("map", MapCmd);

SceneManager::SceneManager(Engine* engine, bool isServer)
    : _engine(engine),
    _scene(new Scene(engine, ""_sid, isServer)),
    _isServer(isServer)
{
    auto emptyMapSegment = new TilemapResource;
    emptyMapSegment->name = "empty-map";
    ResourceManager::GetInstance()->AddResource("empty-map", emptyMapSegment);
}

bool SceneManager::TrySwitchScene(const char* name)
{
    return TrySwitchScene(StringId(name));
}

bool SceneManager::TrySwitchScene(StringId id)
{
    TilemapResource* tilemap = GetResource<TilemapResource>(id, false);
    if (tilemap != nullptr)
    {
        BuildNewScene(tilemap);
        return true;
    }
    else
    {
        Log("Failed to load map %s\n", id.ToString());
        return false;
    }
}

void SceneManager::BuildNewScene(TilemapResource* tilemap)
{
    _scene = std::make_shared<Scene>(_engine, StringId(tilemap->name), _isServer);

    auto screenSize = (_engine->GetSdlManager() != nullptr ? _engine->GetSdlManager()->WindowSize().AsVectorOfType<float>() : Vector2(0, 0));
    _scene->GetCamera()->SetScreenSize(screenSize);
    _scene->GetCamera()->SetZoom(screenSize.y / (1080.0f / 2));

    auto game =_engine->Game();
    auto& config = game->GetConfig();

    _scene->perspective = config.perspective;

    if (config.isMultiplayer)
    {
        _scene->AddService<PlayerCommandRunner>(_scene->isServer);
    }

    if (_scene->SceneName() != "empty-map"_sid)
    {
        game->BuildScene(_scene.get());
    }

    _scene->LoadMapSegment(tilemap->mapSegment);

    _scene->SendEvent(SceneLoadedEvent());
}