#pragma once

class Engine;
class SceneManager;

class IGame
{
public:
    IGame(Engine* engine, SceneManager* sceneManager);

    virtual void PreRender() { }
    virtual void PreUpdate() { }

    Engine* GetEngine() const { return _engine; }
    void Run();

private:
    Engine* _engine;
};