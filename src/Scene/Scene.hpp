
#pragma once

#include <box2d/b2_world.h>
#include <vector>
#include <cassert>
#include <Renderer/Camera.hpp>
#include <memory>
#include <gsl/span>

#include "CameraFollower.hpp"
#include "Entity.hpp"
#include "Renderer/Lighting.hpp"
#include "Memory/FixedSizeVector.hpp"
#include "Memory/FreeList.hpp"
#include "MapSegment.hpp"
#include "Timer.hpp"
#include "Net/ReplicationManager.hpp"

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

struct LoadedSegment
{
    LoadedSegment()
        : segmentId(-1)
    {
        
    }

    LoadedSegment(int segmentId_)
        : segmentId(segmentId_)
    {
        
    }

    void Reset(int segmentId_);
    void AddEntity(Entity* entity);
    void BroadcastEvent(const IEntityEvent& ev);

    bool operator==(const LoadedSegment& rhs) const
    {
        return segmentId == rhs.segmentId;
    }

    int segmentId;
    SegmentLink firstLink;
    SegmentLink lastLink;

    EntityDictionary properties;
};

constexpr int MaxEntities = 8192;

template<typename TInterface>
using EntityList = std::unordered_set<TInterface>;

struct EntityManager
{
    static constexpr int InvalidEntityHeaderId = -2;

    EntityManager()
        : freeEntityHeaders(entityHeaders.begin(), MaxEntities)
    {
        for (int i = 0; i < MaxEntities; ++i)
        {
            entityHeaders[i].id = InvalidEntityHeaderId;
        }
    }

    void RegisterEntity(Entity* entity)
    {
        entities.insert(entity);

        AddIfImplementsInterface(updatables, entity);
        AddIfImplementsInterface(serverUpdatables, entity);

        AddIfImplementsInterface(fixedUpdatables, entity);
        AddIfImplementsInterface(serverFixedUpdatables, entity);

        AddIfImplementsInterface(renderables, entity);
        AddIfImplementsInterface(hudRenderables, entity);

        EntityHeader* header = freeEntityHeaders.Borrow();

        header->id = _nextEntityId++;
        header->entity = entity;

        entity->id = header->id;
        entity->header = header;
    }

    void UnregisterEntity(Entity* entity)
    {
        entities.erase(entity);

        RemoveIfImplementsInterface(updatables, entity);
        RemoveIfImplementsInterface(serverUpdatables, entity);

        RemoveIfImplementsInterface(fixedUpdatables, entity);
        RemoveIfImplementsInterface(serverFixedUpdatables, entity);

        RemoveIfImplementsInterface(renderables, entity);
        RemoveIfImplementsInterface(hudRenderables, entity);

        EntityHeader* header = entity->header;
        header->id = InvalidEntityHeaderId;
        freeEntityHeaders.Return(header);
    }

    template <typename TContainer, typename TItem>
    void AddIfImplementsInterface(EntityList<TContainer>& container, const TItem& item)
    {
        TContainer asContainer;
        if ((asContainer = dynamic_cast<TContainer>(item)) != nullptr)
        {
            container.insert(asContainer);
        }
    }

    template <typename TContainer, typename TItem>
    void RemoveIfImplementsInterface(EntityList<TContainer>& container, const TItem& item)
    {
        TContainer asContainer;
        if ((asContainer = dynamic_cast<TContainer>(item)) != nullptr)
        {
            container.erase(asContainer);
        }
    }

    EntityList<Entity*> entities;
    FreeList<EntityHeader> freeEntityHeaders;

    FixedSizeVector<EntityHeader, MaxEntities> entityHeaders;

    EntityList<IUpdatable*> updatables;
    EntityList<IServerUpdatable*> serverUpdatables;
    EntityList<IFixedUpdatable*> fixedUpdatables;
    EntityList<IServerFixedUpdatable*> serverFixedUpdatables;
    EntityList<IRenderable*> renderables;
    EntityList<IHudRenderable*> hudRenderables;

    std::vector<Entity*> toBeDestroyed;

    int _nextEntityId = 1;
};

class Scene
{
public:
    static constexpr float PhysicsDeltaTime = 1.0f / 200;
    static constexpr Vector2 PixelsToBox2DRatio = Vector2(1.0f / 16, 1.0f / 16);
    static constexpr Vector2 Box2DToPixelsRatio = Vector2(1.0, 1.0) / PixelsToBox2DRatio;
    static constexpr Vector2 Gravity = Vector2(0, 0) * Box2DToPixelsRatio;

    Scene(Engine* engine, StringId mapSegmentName);
    virtual ~Scene();

    static b2Vec2 PixelToBox2D(Vector2 v)
    {
        auto scaled = v * PixelsToBox2DRatio;
        return b2Vec2(scaled.x, scaled.y);
    }

    static Vector2 Box2DToPixel(b2Vec2 v)
    {
        return Vector2(v.x * Box2DToPixelsRatio.x, v.y * Box2DToPixelsRatio.y);
    }

    Engine* GetEngine() const { return _engine; }
    Camera* GetCamera() { return &_camera; }
    CameraFollower* GetCameraFollower() { return &_cameraFollower; }
    b2World* GetWorld() const { return _world.get(); }
    Vector2 SegmentOffset() const { return _segmentOffset; }
    int CurrentSegmentId() const { return _currentSegmentId; }

    Entity* CreateEntity(const EntityDictionary& properties);

    void DestroyEntity(Entity* entity);
    void UpdateEntities(float deltaTime);
    void RenderEntities(Renderer* renderer);
    void RenderHud(Renderer* renderer);

    void ForceFixedUpdate();

    template <typename TService, typename ... Args>
    TService* AddService(Args&& ...args)
    {
        auto service = std::make_unique<TService>(std::forward<Args>(args)...);
        auto servicePtr = service.get();

        service->scene = this;
        service->OnAdded();

        _services.push_back(std::move(service));

        return servicePtr;
    }

    template<typename TService>
    TService* GetService()
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

    void SendEvent(const IEntityEvent& ev)
    {
        ReceiveEvent(ev);

        for (auto& service : _services)
        {
            service->SendEvent(ev, inEditor);
        }
    }

    void BroadcastEvent(const IEntityEvent& ev);

    void SetLightManager(LightManager* lightManager)
    {
        _lightManger = lightManager;
    }

    LightManager* GetLightManager() const
    {
        return _lightManger;
    }

    SoundManager* GetSoundManager() const;

    int LoadMapSegment(StringId id);
    int LoadMapSegment(const MapSegment& segment);
    bool TryGetLoadedSegmentById(const int segmentId, LoadedSegment*& segment);
    void UnloadSegmentsBefore(int segmentId);

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

    StringId MapSegmentName() const { return _mapSegmentName; }

    template<typename TEntity>
    std::vector<TEntity*> GetEntitiesOfType();

    template<typename TEntity>
    TEntity* GetFirstNamedEntityOfType(StringId name);

    std::vector<Entity*> GetEntitiesByNameInSegment(StringId name, int segmentId);

    template<typename TEntity>
    std::vector<TEntity*> GetEntitiesByTypeInSegment(int segmentId);
    Entity* GetFirstEntityByNameInSegment(StringId name, int segmentId);

    template <typename TEntity, typename ... Args>
    TEntity* CreateEntityInternal(const EntityDictionary& properties, Args&& ... constructorArgs);

    virtual void OnSceneLoaded() { }

    auto& GetEntities() { return _entityManager.entities; }

    void* AllocateMemory(int size) const;
    void FreeMemory(void* mem, int size) const;

    std::vector<StringId> segmentsAdded;

    float deltaTime = 0;
    float timeSinceStart = 0;
    TimePointType lastFrameStart;
    int levelId;
    bool isFirstFrame = true;
    bool inEditor = false;
    EntityReference<Entity> soundListener;

    void SetSoundListener(Entity* entity);

    float currentAmbientBrightness = 0.85;
    int currentDifficulty = 1;
    ReplicationManager replicationManager;

protected:
    virtual void ReceiveEvent(const IEntityEvent& ev) { }
    virtual void PreFixedUpdate() { }
    virtual void PostFixedUpdate() { }

    virtual void PreUpdate() { }
    virtual void PostUpdate() { }

    virtual void PreRender() { }
    virtual void PostRender() { }

    virtual void DoRenderHud(Renderer* renderer) { }

private:
    std::vector<std::unique_ptr<ISceneService>> _services;

    void RegisterEntity(Entity* entity, const EntityDictionary& properties);
    void RemoveEntity(Entity* entity);
    void DestroyScheduledEntities();

    int BeginQuery() { return _nextQueryId++; }

    LoadedSegment* RegisterSegment(int segmentId);
    void UnregisterSegment(int segmentId);
    LoadedSegment* GetLoadedSegment(int segmentId);

    StringId _mapSegmentName;

    Camera _camera;
    CameraFollower _cameraFollower;

    Engine* _engine;

    std::unique_ptr<b2World> _world;
    float _physicsTimeLeft = 0;
    
    CollisionManager _collisionManager;
    int _nextQueryId = 0;

    TimerManager _timerManager;

    Vector2 _segmentOffset = Vector2(0, 0);
    Vector2 _entityOffset = Vector2(0, 0);
    int _currentSegmentId = 0;

    LightManager* _lightManger;

    static constexpr int MaxLoadedSegments = 128;
    FixedSizeVector<LoadedSegment*, MaxLoadedSegments> _loadedSegments;
    LoadedSegment _segmentPool[MaxLoadedSegments];
    FreeList<LoadedSegment> _freeSegments;

    EntityManager _entityManager;
};

template <typename TEntity, typename ... Args>
TEntity* Scene::CreateEntityInternal(const EntityDictionary& properties, Args&& ... constructorArgs)
{
    TEntity* entity = (TEntity*)AllocateMemory(sizeof(TEntity));
    entity->scene = this;

    new(entity) TEntity(std::forward<Args>(constructorArgs) ...);
    entity->_position = properties.GetValueOrDefault<Vector2>("position", Vector2(0, 0)) + _entityOffset;
    entity->type = TEntity::Type;

    RegisterEntity(entity, properties);

    return entity;
}

template <typename TEntity>
std::vector<TEntity*> Scene::GetEntitiesOfType()
{
    std::vector<TEntity*> foundEntities;

    for(auto entity : _entities)
    {
        TEntity* tEntity;
        if(entity->Is<TEntity>(tEntity))
        {
            foundEntities.push_back(tEntity);
        }
    }

    return foundEntities;
}

template <typename TEntity>
std::vector<TEntity*> Scene::GetEntitiesByTypeInSegment(int segmentId)
{
    std::vector<TEntity*> foundEntities;

    auto segment = GetLoadedSegment(segmentId);

    if(segment == nullptr)
    {
        return { };
    }

    for (auto node = segment->firstLink.next; node != &segment->lastLink; node = node->next)
    {
        TEntity* tEntity;
        auto entity = node->GetEntity();

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
