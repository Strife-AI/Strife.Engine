#include "Scene.hpp"

#include <box2d/b2_fixture.h>
#include <box2d/b2_world.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_edge_shape.h>


#include "AmbientLightEntity.hpp"
#include "Engine.hpp"
#include "Physics/Physics.hpp"
#include "Renderer.hpp"
#include "SceneManager.hpp"
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
    DestroyScheduledEntities();

    for(auto entity : _entityManager.entities)
    {
        DestroyEntity(entity);
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

    
    entity->SegmentLink::Unlink();

    _entityManager.UnregisterEntity(entity);

    auto memoryBlock = entity->GetMemoryBlock();
    entity->~Entity();
    FreeMemory(memoryBlock.second, memoryBlock.first);
}

Entity* Scene::CreateEntity(const EntityDictionary& properties)
{
    auto result = EntityUtil::EntityMetadata::CreateEntityFromType(this, properties);

    return result;
}

void Scene::DestroyEntity(Entity* entity)
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

    while (toBeDestroyed.size() != 0)
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

static Vector2 ToVector2(b2Vec2 v)
{
    return Vector2(v.x, v.y);
}

static b2Vec2 ToB2Vec2(Vector2 v)
{
    return b2Vec2(v.x, v.y);
}

ConsoleVar<bool> g_drawColliders("colliders", false);

void Scene::RenderEntities(Renderer* renderer)
{
    PreRender();
    SendEvent(PreRenderEvent());

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

    PostRender();

    if (g_drawColliders.Value())
    {
        for (auto body = _world->GetBodyList(); body != nullptr; body = body->GetNext())
        {
            for (auto fixture = body->GetFixtureList(); fixture != nullptr; fixture = fixture->GetNext())
            {
                if (fixture->IsSensor()) continue;;

                auto shape = fixture->GetShape();
                switch (shape->GetType())
                {
                case b2Shape::e_circle:
                {
                    auto circle = static_cast<b2CircleShape*>(shape);

                    renderer->RenderCircleOutline(Scene::Box2DToPixel(body->GetWorldPoint(circle->m_p)), circle->m_radius * Scene::Box2DToPixelsRatio.x, Color(0, 255, 0), 0);// FIXME MW DebugRenderLayer);
                    break;
                }
                case b2Shape::e_edge:
                {
                    auto edge = static_cast<b2EdgeShape*>(shape);
                    renderer->RenderLine(
                        Box2DToPixel(body->GetWorldPoint(edge->m_vertex1)),
                        Box2DToPixel(body->GetWorldPoint(edge->m_vertex2)),
                        Color(255, 255, 255),
                        0);//DebugRenderLayer); FIXME MW

                    break;
                }
                case b2Shape::e_polygon:
                {
                    auto polygon = static_cast<b2PolygonShape*>(shape);

                    for (int i = 0; i < polygon->m_count; ++i)
                    {
                        auto vertex = ToVector2(polygon->m_vertices[i] + body->GetPosition()) * Scene::Box2DToPixelsRatio;
                        auto next = ToVector2(polygon->m_vertices[(i + 1) % polygon->m_count] + body->GetPosition()) * Scene::Box2DToPixelsRatio;

                        renderer->RenderLine(vertex, next, Color::Red(), 0);// FIXME MW DebugRenderLayer);
                    }

                    break;
                }
                case b2Shape::e_chain: break;
                case b2Shape::e_typeCount: break;
                default:;
                }
            }
        }
    }
}

void Scene::RenderHud(Renderer* renderer)
{
    for (auto hudRenderable : _entityManager.hudRenderables)
    {
        hudRenderable->RenderHud(renderer);
    }

    DoRenderHud(renderer);
    SendEvent(RenderHudEvent(renderer));
}

void Scene::ForceFixedUpdate()
{
    _world->Step(PhysicsDeltaTime, 8, 3);
    _collisionManager.UpdateEntityPositions();
}

struct FindFixturesQueryCallback : b2QueryCallback
{
    FindFixturesQueryCallback(gsl::span<ColliderHandle> foundFixtures_)
        : foundFixtures(foundFixtures_)
    {

    }

    bool ReportFixture(b2Fixture* fixture) override
    {
        if (count < foundFixtures.size())
        {
            foundFixtures[count++] = ColliderHandle(fixture);
            return true;
        }
        else
        {
            return false;
        }
    }

    gsl::span<ColliderHandle> Results() const
    {
        return foundFixtures.subspan(0, count);
    }

    gsl::span<ColliderHandle> foundFixtures;
    int count = 0;
};


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

void CameraCommand(ConsoleCommandBinder& binder)
{
    binder.Help("Shows info about camera");

    auto position = Engine::GetInstance()->GetSceneManager()->GetScene()->GetCamera()->Position();
    auto bounds = Engine::GetInstance()->GetSceneManager()->GetScene()->GetCamera()->Bounds();

    binder.GetConsole()->Log("Position: %f %f\n", position.x, position.y);
    binder.GetConsole()->Log("TopLeft: %f %f\n", bounds.TopLeft().x, bounds.TopLeft().y);
    binder.GetConsole()->Log("BottomRight: %f %f\n", bounds.BottomRight().x, bounds.BottomRight().y);

}

ConsoleCmd cameraCmd("camera", CameraCommand);

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
        auto entity = CreateEntity(properties);

        if (entity->flags & PreventUnloading)
        {
            entity->Unlink();
        }
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
    aabb.lowerBound = ToB2Vec2(bounds.TopLeft() * PixelsToBox2DRatio);
    aabb.upperBound = ToB2Vec2(bounds.BottomRight() * PixelsToBox2DRatio);

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

extern ConsoleVar<bool> g_stressTest;

void Scene::UpdateEntities(float deltaTime)
{
    _physicsTimeLeft += deltaTime;
    timeSinceStart += deltaTime;

    if (_physicsTimeLeft > 1)
    {
        // We're more than a second behind, it's better to just drop the time so we don't get too far behind
        // trying to catch up
        _physicsTimeLeft = 0;
    }

    while (_physicsTimeLeft >= PhysicsDeltaTime)
    {
        _physicsTimeLeft -= PhysicsDeltaTime;
        for (auto fixedUpdatable : _entityManager.fixedUpdatables)
        {
            fixedUpdatable->FixedUpdate(PhysicsDeltaTime);
        }

        if(_engine->GetNetworkManger()->IsServer())
        {
            for(auto serverFixedUpdatable : _entityManager.serverFixedUpdatables)
            {
                serverFixedUpdatable->ServerFixedUpdate(PhysicsDeltaTime);
            }
        }

        SendEvent(FixedUpdateEvent());

        _world->Step(PhysicsDeltaTime, 8, 3);

        _collisionManager.UpdateEntityPositions();
    }

    PreUpdate();
    SendEvent(PreUpdateEvent());

    for (auto updatable : _entityManager.updatables)
    {
        updatable->Update(deltaTime);
    }

    if (_engine->GetNetworkManger()->IsServer())
    {
        for (auto serverUpdatable : _entityManager.serverUpdatables)
        {
            serverUpdatable->ServerUpdate(deltaTime);
        }
    }

    _timerManager.TickTimers(deltaTime);

    PostUpdate();


    DestroyScheduledEntities();

    _cameraFollower.Update(deltaTime);
}