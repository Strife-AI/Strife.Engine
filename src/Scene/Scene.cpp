#include "Scene.hpp"
#include "Engine.hpp"
#include "Physics/Physics.hpp"
#include "Renderer.hpp"
#include "SdlManager.hpp"
#include "Physics/PathFinding.hpp"
#include "Scene/TilemapEntity.hpp"
#include "Tools/Console.hpp"

Scene::Scene(Engine* engine, StringId mapSegmentName)
    : replicationManager(this, engine->GetNetworkManger() != nullptr ? engine->GetNetworkManger()->IsServer() : false),
    _mapSegmentName(mapSegmentName),
    _cameraFollower(&_camera, engine->GetInput()),
    _engine(engine),
    _world(std::make_unique<b2World>(b2Vec2(0, 0))),
    _collisionManager(_world.get())
{
    _world->SetContactListener(&_collisionManager);
    _camera.SetScreenSize(engine->GetSdlManager()->WindowSize().AsVectorOfType<float>());
}

Scene::~Scene()
{
    for (auto entity : _entityManager.entities)
    {
        MarkEntityForDestruction(entity);
    }

    DestroyScheduledEntities();
}

void Scene::RegisterEntity(Entity* entity, const EntityDictionary& properties)
{
    entity->_dimensions = properties.GetValueOrDefault<Vector2>("dimensions", Vector2(0, 0));
    entity->_rotation = properties.GetValueOrDefault<float>("rotation", 0);

    std::string_view name = properties.GetValueOrDefault<std::string_view>("name", "");
    entity->name = StringId(name);

    _entityManager.RegisterEntity(entity);

    entity->scene = this;
    _engine->GetSoundManager()->AddSoundEmitter(&entity->_soundEmitter, entity);

    entity->OnAdded(properties);
}

void Scene::RemoveEntity(Entity* entity)
{
    entity->OnDestroyed();
    _engine->GetSoundManager()->RemoveSoundEmitter(&entity->_soundEmitter);

    // Remove components
    {
        for (auto component = entity->_componentList; component != nullptr;)
        {
            auto next = component->next;
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

Entity* Scene::CreateEntity(const EntityDictionary& properties)
{
    auto result = EntityUtil::EntityMetadata::CreateEntityFromType(this, properties);

    return result;
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
    GetSoundManager()->SetListenerPosition(entity->Center(), { 0, 0});
}

ConsoleVar<bool> g_drawColliders("colliders", false);

void Scene::RenderEntities(Renderer* renderer)
{
    SendEvent(RenderEvent(renderer));

    for (auto renderable : _entityManager.renderables)
    {
        renderable->Render(renderer);
    }

    // TODO: more efficient way of rendering components
    for(auto entity : _entityManager.entities)
    {
        for(IEntityComponent* component = entity->_componentList; component != nullptr; component = component->next)
        {
            component->Render(renderer);
        }
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

void Scene::BroadcastEvent(const IEntityEvent& ev)
{
    for (auto entity : _entityManager.entities)
    {
        entity->SendEvent(ev);
    }

    SendEvent(ev);
}

SoundManager* Scene::GetSoundManager() const
{
    return _engine->GetSoundManager();
}

void Scene::LoadMapSegment(StringId id)
{
    auto mapSegment = ResourceManager::GetResource<MapSegment>(id);
    LoadMapSegment(*mapSegment.Value());
}

void Scene::LoadMapSegment(const MapSegment& segment)
{
    EntityDictionary tilemapProperties
    {
        { "type", "tilemap"_sid }
    };

    auto tileMap = CreateEntityInternal<TilemapEntity>(tilemapProperties);
    tileMap->SetMapSegment(segment);

    for (auto& instance : segment.entities)
    {
        EntityDictionary properties(instance.properties.get(), instance.totalProperties);
        CreateEntity(properties);
    }
}

void Scene::StartTimer(float timeSeconds, const std::function<void()>& callback)
{
    _timerManager.StartTimer(timeSeconds, callback);
}

void Scene::StartEntityTimer(float timeSeconds, const std::function<void()>& callback, Entity* entity)
{
    _timerManager.StartEntityTimer(timeSeconds, callback, entity);
}

gsl::span<ColliderHandle> Scene::FindOverlappingColliders(const Rectangle& bounds, gsl::span<ColliderHandle> storage) const
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

        if (entity->_lastQueryId != queryId)
        {
            storage[totalOverlappingEntities++] = entity;
            entity->_lastQueryId = queryId;
        }
    }

    return storage.subspan(0, totalOverlappingEntities);
}

bool Scene::Raycast(Vector2 start, Vector2 end, RaycastResult& outResult, bool allowTriggers, const std::function<bool(ColliderHandle handle)>& includeFixture) const
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
    timeSinceStart += deltaTime;

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
    _timerManager.TickTimers(deltaTime);

    _cameraFollower.Update(deltaTime);

    DestroyScheduledEntities();
}

void Scene::NotifyFixedUpdate()
{
    SendEvent(FixedUpdateEvent());

    for (auto fixedUpdatable : _entityManager.fixedUpdatables)
    {
        fixedUpdatable->FixedUpdate(PhysicsDeltaTime);
    }
}

void Scene::NotifyServerFixedUpdate()
{
    if (_engine->GetNetworkManger()->IsServer())
    {
        for (auto serverFixedUpdatable : _entityManager.serverFixedUpdatables)
        {
            serverFixedUpdatable->ServerFixedUpdate(PhysicsDeltaTime);
        }
    }
}

void Scene::NotifyUpdate(float deltaTime)
{
    for (auto updatable : _entityManager.updatables)
    {
        updatable->Update(deltaTime);
    }
}

void Scene::NotifyServerUpdate(float deltaTime)
{
    if (_engine->GetNetworkManger()->IsServer())
    {
        for (auto serverUpdatable : _entityManager.serverUpdatables)
        {
            serverUpdatable->ServerUpdate(deltaTime);
        }
    }
}