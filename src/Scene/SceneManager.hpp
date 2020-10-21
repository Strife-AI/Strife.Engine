#pragma once

struct EntityInstance;
struct MapSegment;
class Engine;
class StringId;
class Scene;
struct Entity;

class SceneManager
{
public:
    SceneManager(Engine* engine);
    virtual ~SceneManager() = default;

    Scene* GetScene() const { return _scene; }
    Scene* GetNewScene() const { return _newScene; }
    bool TrySwitchScene(StringId name);
    virtual void StartNewScene() = 0;

    void DoSceneTransition();

protected:
    virtual Scene* BuildScene(StringId name) = 0;
    virtual void PostBuildScene(Scene* scene) { }
    virtual void PreDestroyScene(Scene* scene) { }

    Engine* _engine;

private:
    void BuildNewScene(const MapSegment* map);
    void DestroyScene();

    Scene* _scene = nullptr;
    Scene* _newScene = nullptr;
};
