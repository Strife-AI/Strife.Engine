#include <chrono>
#include <Resource/TilemapResource.hpp>
#include <Net/PlayerCommandRunner.hpp>
#include "Tools/Console.hpp"
#include "Engine.hpp"

#include "SceneManager.hpp"

#include "IGame.hpp"
#include "Scene.hpp"
#include "SdlManager.hpp"
#include "IEntityEvent.hpp"
#include "Project/Project.hpp"

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
    auto scene = _engine->Game()->project->scenes.find(id);

    if (scene != _engine->Game()->project->scenes.end())
    {
        SceneModel& model = _engine->Game()->project->scenes[id];
        BuildNewScene(&model);
        return true;
    }
    else
    {
        Log("Failed to load map %s\n", id.ToString());
        return false;
    }
}

void SceneManager::BuildNewScene(const SceneModel* sceneModel)
{
    _scene = std::make_shared<Scene>(_engine, StringId(sceneModel->sceneName), _isServer);

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

    game->BuildScene(_scene.get());

    for (auto& entity : sceneModel->entities)
    {
        EntitySerializer serializer {
            EntitySerializerMode::Read,
            entity.properties
        };

        if (entity.type == "tilemap"_sid)
        {
            TilemapResource* mapSegment = GetResource<TilemapResource>(entity.name.c_str(), false);
            _scene->LoadMapSegment(mapSegment->mapSegment);
        }
        else
        {
            _scene->CreateEntity(entity.type, serializer);
        }
    }

    _scene->SendEvent(SceneLoadedEvent());
}