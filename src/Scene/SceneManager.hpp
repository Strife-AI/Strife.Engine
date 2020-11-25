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
    SceneManager(Engine* engine, bool isServer);
    virtual ~SceneManager() = default;

    Scene* GetScene() const { return _scene; }
    Scene* GetNewScene() const { return _newScene; }
    bool TrySwitchScene(StringId name);
    void DoSceneTransition();

protected:

    Engine* _engine;

private:
    friend struct BaseGameInstance;

    void BuildNewScene(const MapSegment* map);
    void DestroyScene();

    Scene* _scene = nullptr;
    Scene* _newScene = nullptr;
    bool _isServer = false;
};
