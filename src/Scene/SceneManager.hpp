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

    Scene* GetScene() const { return _scene.get(); }
    std::shared_ptr<Scene> GetSceneShared() const { return _scene; }
    bool TrySwitchScene(StringId name);

protected:

    Engine* _engine;

private:
    friend struct BaseGameInstance;

    void BuildNewScene(const MapSegment* map);

    std::shared_ptr<Scene> _scene = nullptr;
    bool _isServer = false;
};
