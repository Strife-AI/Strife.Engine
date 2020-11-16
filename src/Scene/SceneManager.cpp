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

    binder
        .Bind(mapName, "map-name")
        .Help("Change scene to specified map.");

    auto sceneManager = Engine::GetInstance()->GetSceneManager();
    if(!sceneManager->TrySwitchScene(StringId(mapName.c_str())))
    {
        Engine::GetInstance()->GetConsole()->Log("Failed to execute scene transition\n");
        return;
    }

    Log("Map: %s\n", mapName.c_str());
}
ConsoleCmd mapCmd("map", MapCmd);

void SegmentCmd(ConsoleCommandBinder& binder)
{
    std::string segmentName;

    binder
        .Bind(segmentName, "segmentName")
        .Help("Appends a segment to the end of the level");

    auto sceneManager = Engine::GetInstance()->GetSceneManager();
    auto scene = sceneManager->GetNewScene() != nullptr ? sceneManager->GetNewScene() : sceneManager->GetScene();

    scene->LoadMapSegment(StringId(segmentName.c_str()));
}
ConsoleCmd segmentCmd("segment", SegmentCmd);

SceneManager::SceneManager(Engine* engine)
    : _engine(engine),
    _scene(new Scene(engine, ""_sid))
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

        _scene->lastFrameStart = std::chrono::high_resolution_clock::now();
        _newScene = nullptr;

        _scene->OnSceneLoaded();
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

    _newScene = new Scene(_engine, mapSegment->name);
    _newScene->SetLightManager(_engine->GetRenderer()->GetLightManager());

    auto screenSize = _engine->GetSdlManager()->WindowSize().AsVectorOfType<float>();
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
