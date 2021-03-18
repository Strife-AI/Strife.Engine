#include <Resource/ResourceManager.hpp>
#include <Resource/TilemapResource.hpp>
#include "Scene.hpp"
#include "Engine.hpp"
#include "Physics/Physics.hpp"
#include "Renderer.hpp"
#include "SdlManager.hpp"
#include "Physics/PathFinding.hpp"
#include "Scene/TilemapEntity.hpp"
#include "Tools/Console.hpp"
#include "Net/ReplicationManager.hpp"

Entity* Scene::entityUnderConstruction = nullptr;

Scene::Scene(Engine* engine, StringId mapSegmentName, bool isServer)
    : isServer(isServer),
      _sceneName(mapSegmentName),
      _cameraFollower(&_camera, engine->GetInput()),
      _engine(engine),
      _world(std::make_unique<b2World>(b2Vec2(0, 0))),
      _collisionManager(_world.get()),
      isometricSettings(this)
{
    _world->SetContactListener(&_collisionManager);
    _camera.SetScreenSize(engine->GetSdlManager() == nullptr ? Vector2(0, 0)
                                                             : engine->GetSdlManager()->WindowSize().AsVectorOfType<float>());
    replicationManager = AddService<ReplicationManager>(this, isServer);
}

Scene::~Scene()
{
    for (auto entity : _entityManager.entities)
    {
        MarkEntityForDestruction(entity);
    }

    DestroyScheduledEntities();
}

b2Vec2 Scene::PixelToBox2D(Vector2 v)
{
    auto scaled = v * PixelsToBox2DRatio;
    return b2Vec2(scaled.x, scaled.y);
}

Vector2 Scene::Box2DToPixel(b2Vec2 v)
{
    return Vector2(v.x * Box2DToPixelsRatio.x, v.y * Box2DToPixelsRatio.y);
}

void Scene::RegisterEntity(Entity* entity)
{
    _entityManager.RegisterEntity(entity);
    _engine->GetSoundManager()->AddSoundEmitter(&entity->_soundEmitter, entity);
    _entityManager.AddInterfaces(entity);
}

void Scene::RemoveEntity(Entity* entity)
{
    entity->OnDestroyed();
    _engine->GetSoundManager()->RemoveSoundEmitter(&entity->_soundEmitter);

    // Remove components
    {
        for (auto component = entity->_componentList; component != nullptr;)
        {
            _componentManager.Unregister(component);
            auto next = component->next;

            // Prevent any events from being sent after it's been destroyed e.g. ContactEndEvent
            entity->_componentList = next;

            component->OnRemoved();
            auto block = component->GetMemoryBlock();
            FreeMemory(block.second, block.first);
            component = next;
        }

        entity->_componentList = nullptr;
    }

    _entityManager.UnregisterEntity(entity);

    // Free memory
    {
        auto memoryBlock = entity->GetMemoryBlock();
        entity->~Entity();
        FreeMemory(memoryBlock.second, memoryBlock.first);
    }
}

void Scene::MarkEntityForDestruction(Entity* entity)
{
    if (entity->isDestroyed)
    {
        return;
    }

    entity->isDestroyed = true;

    _entityManager.toBeDestroyed.push_back(entity);
}

void Scene::DestroyScheduledEntities()
{
    auto& toBeDestroyed = _entityManager.toBeDestroyed;

    while (!toBeDestroyed.empty())
    {
        auto entityToDestroy = toBeDestroyed[toBeDestroyed.size() - 1];
        toBeDestroyed.pop_back();
        RemoveEntity(entityToDestroy);
    }
}

void* Scene::AllocateMemory(int size) const
{
    return _engine->GetDefaultBlockAllocator()->Allocate(size);
}

void Scene::FreeMemory(void* mem, int size) const
{
    _engine->GetDefaultBlockAllocator()->Free(mem, size);
}

void Scene::SetSoundListener(Entity* entity)
{
    soundListener = entity;
    GetSoundManager()->SetListenerPosition(entity->Center(), { 0, 0 });
}

ConsoleVar<bool> g_drawColliders("colliders", false);

void Scene::RenderEntities(Renderer* renderer)
{
    SendEvent(RenderEvent(renderer));

    for (auto renderable : _entityManager.renderables)
    {
        renderable->Render(renderer);
    }

    for (auto component : _componentManager.renderables)
    {
        component->Render(renderer);
    }

    if (g_drawColliders.Value())
    {
        _collisionManager.RenderColliderOutlines(renderer);
    }
}

void Scene::RenderHud(Renderer* renderer)
{
    for (auto hudRenderable : _entityManager.hudRenderables)
    {
        hudRenderable->RenderHud(renderer);
    }

    SendEvent(RenderHudEvent(renderer));
}

void Scene::StepPhysicsSimulation()
{
    _world->Step(PhysicsDeltaTime, 8, 3);
    _collisionManager.UpdateEntityPositions();
}

void Scene::SendEvent(const IEntityEvent& ev)
{
    for (auto& service : _services)
    {
        service->SendEvent(ev, inEditor);
    }
}

void Scene::BroadcastEvent(const IEntityEvent& ev)
{
    for (auto entity : _entityManager.entities)
    {
        entity->SendEvent(ev);
    }

    SendEvent(ev);
}

LightManager* Scene::GetLightManager() const
{
    return _engine->GetRenderer()->GetLightManager();
}

SoundManager* Scene::GetSoundManager() const
{
    return _engine->GetSoundManager();
}

void Scene::LoadMapSegment(const char* name)
{
    auto mapSegment = GetResource<TilemapResource>(name);
    LoadMapSegment(mapSegment->mapSegment);
}

void Scene::LoadMapSegment(const MapSegment& segment)
{
    auto tileMap = CreateEntity<TilemapEntity>({ 0, 0 });
    tileMap->SetMapSegment(segment);

}

void Scene::StartTimer(float timeSeconds, const std::function<void()>& callback)
{
    _timerManager.StartTimer(timeSeconds, callback);
}

void Scene::StartEntityTimer(float timeSeconds, const std::function<void()>& callback, Entity* entity)
{
    _timerManager.StartEntityTimer(timeSeconds, callback, entity);
}

gsl::span<ColliderHandle>
Scene::FindOverlappingColliders(const Rectangle& bounds, gsl::span<ColliderHandle> storage) const
{
    FindFixturesQueryCallback callback(storage);
    b2AABB aabb;
    aabb.lowerBound = PixelToBox2D(bounds.TopLeft());
    aabb.upperBound = PixelToBox2D(bounds.BottomRight());

    _world->QueryAABB(&callback, aabb);

    return callback.Results();
}

gsl::span<Entity*> Scene::FindOverlappingEntities(const Rectangle& bounds, gsl::span<Entity*> storage)
{
    static ColliderHandle overlappingColliders[MaxEntities * 4];
    auto colliders = FindOverlappingColliders(bounds, overlappingColliders);

    int totalOverlappingEntities = 0;
    int queryId = BeginQuery();

    for (const auto& collider : colliders)
    {
        auto entity = collider.OwningEntity();

        if (entity->lastQueryId != queryId)
        {
            storage[totalOverlappingEntities++] = entity;
            entity->lastQueryId = queryId;
        }
    }

    return storage.subspan(0, totalOverlappingEntities);
}

bool Scene::Raycast(Vector2 start, Vector2 end, RaycastResult& outResult, bool allowTriggers,
    const std::function<bool(ColliderHandle handle)>& includeFixture) const
{
    if (IsApproximately((end - start).TaxiCabDistance(), 0))
    {
        return false;
    }

    FindClosestRaycastCallback callback(allowTriggers, includeFixture);

    _world->RayCast(&callback, PixelToBox2D(start), PixelToBox2D(end));

    if (callback.lastFraction == 1)
    {
        return false;
    }
    else
    {
        outResult.point = Box2DToPixel(callback.closestPoint);
        outResult.distance = (outResult.point - start).Length();
        outResult.handle = ColliderHandle(callback.closestFixture);
        outResult.normal = Box2DToPixel(callback.closestNormal).Normalize();

        return true;
    }
}

void Scene::UpdateEntities(float deltaTime)
{
    _physicsTimeLeft += deltaTime;
    relativeTime += deltaTime;

    while (_physicsTimeLeft >= PhysicsDeltaTime)
    {
        _physicsTimeLeft -= PhysicsDeltaTime;

        NotifyFixedUpdate();
        NotifyServerFixedUpdate();
        StepPhysicsSimulation();
    }

    SendEvent(UpdateEvent());

    NotifyUpdate(deltaTime);
    NotifyServerUpdate(deltaTime);

    SendEvent(EndOfUpdateEvent());

    _timerManager.TickTimers(deltaTime);

    _cameraFollower.Update(deltaTime);

    _entityManager.UpdateInterfaces();
    _componentManager.UpdateScheduledComponents();
    DestroyScheduledEntities();
}

void Scene::NotifyFixedUpdate()
{
    SendEvent(FixedUpdateEvent());

    if (!isServer)
    {
        for (auto fixedUpdatable : _entityManager.fixedUpdatables)
        {
            fixedUpdatable->FixedUpdate(PhysicsDeltaTime);
        }
    }

    for (auto component : _componentManager.fixedUpdatables)
    {
        component->FixedUpdate(deltaTime);
    }
}

void Scene::NotifyServerFixedUpdate()
{
    if (isServer)
    {
        for (auto serverFixedUpdatable : _entityManager.serverFixedUpdatables)
        {
            serverFixedUpdatable->ServerFixedUpdate(PhysicsDeltaTime);
        }
    }
}

void Scene::NotifyUpdate(float deltaTime)
{
    if (!isServer)
    {
        for (auto updatable : _entityManager.updatables)
        {
            updatable->Update(deltaTime);
        }
    }

    for (auto component : _componentManager.updatables)
    {
        component->Update(deltaTime);
    }
}

void Scene::NotifyServerUpdate(float deltaTime)
{
    if (isServer)
    {
        for (auto serverUpdatable : _entityManager.serverUpdatables)
        {
            serverUpdatable->ServerUpdate(deltaTime);
        }
    }
}

Entity* Scene::CreateEntity(StringId type, EntitySerializer& serializer)
{
    return EntityUtil::EntityMetadata::CreateEntityFromType(type, this, serializer);
}