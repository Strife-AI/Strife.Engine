
#pragma once

#include <box2d/b2_world.h>
#include <vector>
#include <Renderer/Camera.hpp>
#include <memory>
#include <gsl/span>

#include "CameraFollower.hpp"
#include "Entity.hpp"
#include "EntityManager.hpp"
#include "Renderer/Lighting.hpp"
#include "Memory/FreeList.hpp"
#include "MapSegment.hpp"
#include "Timer.hpp"

class StringId;
class Renderer;
class Engine;
class b2World;
struct IEntityFactory;

enum GameServiceFlags
{
    ActiveInEditor = 1
};

struct ISceneService
{
    void SendEvent(const IEntityEvent& ev, bool inEditor)
    {
        bool activeInEditor = flags & ActiveInEditor;
        if(activeInEditor || !inEditor)
        {
            ReceiveEvent(ev);
        }
    }

    virtual void OnAdded() { }

    virtual ~ISceneService() = default;

    int flags = 0;
    Scene* scene = nullptr;

private:
    virtual void ReceiveEvent(const IEntityEvent& ev) = 0;
};

struct RaycastResult
{
    ColliderHandle handle;
    float distance;
    Vector2 point;
    Vector2 normal;
};

class ReplicationManager;

class Scene
{
public:
    static constexpr float PhysicsDeltaTime = 1.0f / 200;
    static constexpr Vector2 PixelsToBox2DRatio = Vector2(1.0f / 16, 1.0f / 16);
    static constexpr Vector2 Box2DToPixelsRatio = Vector2(1.0, 1.0) / PixelsToBox2DRatio;
    static constexpr Vector2 Gravity = Vector2(0, 0) * Box2DToPixelsRatio;

    Scene(Engine* engine, StringId mapSegmentName, bool isServer);
    ~Scene();

    static b2Vec2 PixelToBox2D(Vector2 v);
    static Vector2 Box2DToPixel(b2Vec2 v);

    Engine* GetEngine() const { return _engine; }
    Camera* GetCamera() { return &_camera; }
    CameraFollower* GetCameraFollower() { return &_cameraFollower; }
    b2World* GetWorld() const { return _world.get(); }
    StringId MapSegmentName() const { return _mapSegmentName; }
    auto& GetEntities() { return _entityManager.entities; }

    void SetSoundListener(Entity* entity);

    Entity* CreateEntity(const EntityDictionary& properties);

    void StepPhysicsSimulation();

    template <typename TService, typename ... Args> TService*   AddService(Args&& ...args);
    template<typename TService> TService*                       GetService();

    void SendEvent(const IEntityEvent& ev);
    void BroadcastEvent(const IEntityEvent& ev);

    LightManager* GetLightManager() const;
    SoundManager* GetSoundManager() const;

    void LoadMapSegment(StringId id);
    void LoadMapSegment(const MapSegment& segment);

    void StartTimer(float timeSeconds, const std::function<void()>& callback);
    void StartEntityTimer(float timeSeconds, const std::function<void()>& callback, Entity* entity);

    gsl::span<ColliderHandle> FindOverlappingColliders(const Rectangle& bounds, gsl::span<ColliderHandle> storage) const;
    gsl::span<Entity*> FindOverlappingEntities(const Rectangle& bounds, gsl::span<Entity*> storage);
    bool Raycast(
        Vector2 start,
        Vector2 end,
        RaycastResult& outResult,
        bool allowTriggers = false,
        const std::function<bool(ColliderHandle handle)>& includeFixture = nullptr) const;

    template<typename TEntity> std::vector<TEntity*> GetEntitiesOfType();
    template<typename TEntity> TEntity* GetFirstNamedEntityOfType(StringId name);

    template <typename TEntity, typename ... Args>
    TEntity* CreateEntityInternal(const EntityDictionary& properties, Args&& ... constructorArgs);

    void* AllocateMemory(int size) const;
    void FreeMemory(void* mem, int size) const;

    EntityManager& GetEntityManager() { return _entityManager; }

    float deltaTime = 0;
    float relativeTime = 0;
    float absoluteTime = 0;
    float lastFrameStart;
    bool isFirstFrame = true;
    bool inEditor = false;
    EntityReference<Entity> soundListener;
    ReplicationManager* replicationManager;
    bool isServer;

    static Entity* entityUnderConstruction;

private:
    friend struct Entity;
    friend class BaseGameInstance;

    std::vector<std::unique_ptr<ISceneService>> _services;

    void MarkEntityForDestruction(Entity* entity);
    void RegisterEntity(Entity* entity, const EntityDictionary& properties);
    void RemoveEntity(Entity* entity);
    void DestroyScheduledEntities();

    void UpdateEntities(float deltaTime);
    void RenderEntities(Renderer* renderer);
    void RenderHud(Renderer* renderer);

    void NotifyUpdate(float deltaTime);
    void NotifyServerUpdate(float deltaTime);
    void NotifyFixedUpdate();
    void NotifyServerFixedUpdate();

    int BeginQuery() { return _nextQueryId++; }

    StringId _mapSegmentName;

    Camera _camera;
    CameraFollower _cameraFollower;

    Engine* _engine;

    std::unique_ptr<b2World> _world;
    float _physicsTimeLeft = 0;
    CollisionManager _collisionManager;
    int _nextQueryId = 0;

    TimerManager _timerManager;
    EntityManager _entityManager;
};

#define CHECK_OVERRIDE(name_, flag_) if (&TEntity::name_ != &Entity::name_) entity->flags.SetFlag(flag_)

template <typename TEntity, typename ... Args>
TEntity* Scene::CreateEntityInternal(const EntityDictionary& properties, Args&& ... constructorArgs)
{
    auto oldEntityUnderConstruction = entityUnderConstruction;

    TEntity* entity = (TEntity*)AllocateMemory(sizeof(TEntity));

    entity->scene = this;

    entityUnderConstruction = entity;
    new(entity) TEntity(std::forward<Args>(constructorArgs) ...);

    entity->_position = properties.GetValueOrDefault<Vector2>("position", Vector2(0, 0));
    entity->type = TEntity::Type;

    CHECK_OVERRIDE(Update, EntityFlags::EnableUpdate);
    CHECK_OVERRIDE(FixedUpdate, EntityFlags::EnableFixedUpdate);
    CHECK_OVERRIDE(ServerUpdate, EntityFlags::EnableServerUpdate);
    CHECK_OVERRIDE(ServerFixedUpdate, EntityFlags::EnableServerFixedUpdate);
    CHECK_OVERRIDE(Render, EntityFlags::EnableRender);
    CHECK_OVERRIDE(RenderHud, EntityFlags::EnableRenderHud);

    if(&TEntity::Update != &Entity::Update)
	{
    	Log("Different functions\n");
	}

    RegisterEntity(entity, properties);
    entityUnderConstruction = oldEntityUnderConstruction;

    return entity;
}

#undef CHECK_OVERRIDE

template <typename TService, typename ... Args>
TService* Scene::AddService(Args&&... args)
{
    auto service = std::make_unique<TService>(std::forward<Args>(args)...);
    auto servicePtr = service.get();

    service->scene = this;
    service->OnAdded();

    _services.push_back(std::move(service));

    return servicePtr;
}

template <typename TService>
TService* Scene::GetService()
{
    for (auto& service : _services)
    {
        if (auto servicePtr = dynamic_cast<TService*>(service.get()))
        {
            return servicePtr;
        }
    }

    FatalError("Missing service");
}

template <typename TEntity>
std::vector<TEntity*> Scene::GetEntitiesOfType()
{
    std::vector<TEntity*> foundEntities;

    for(auto entity : _entityManager.entities)
    {
        TEntity* tEntity;
        if(entity->Is<TEntity>(tEntity))
        {
            foundEntities.push_back(tEntity);
        }
    }

    return foundEntities;
}

template<typename TEntity>
TEntity* Scene::GetFirstNamedEntityOfType(StringId name)
{
    auto entities = GetEntitiesOfType<TEntity>();
    for (auto& entity : entities)
    {
        if (entity->name == name)
        {
            return entity;
        }
    }

    return nullptr;
}
