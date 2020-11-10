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
#include "Scene/TilemapEntity.hpp"
#include "Tools/Console.hpp"

template <typename TContainer, typename TItem, int MaxSize>
void AddIfImplementsInterface(FixedSizeVector<TContainer, MaxSize>& container, const TItem& item)
{
    TContainer asContainer;
    if ((asContainer = dynamic_cast<TContainer>(item)) != nullptr)
    {
        container.PushBackUnique(asContainer);
    }
}

template <typename TContainer, typename TItem, int MaxSize>
void RemoveIfImplementsInterface(FixedSizeVector<TContainer, MaxSize>& container, const TItem& item)
{
    TContainer asContainer;
    if ((asContainer = dynamic_cast<TContainer>(item)) != nullptr)
    {
        container.RemoveSingle(asContainer);
    }
}

void LoadedSegment::Reset(int segmentId_)
{
    segmentId = segmentId_;

    firstLink.next = &lastLink;
    firstLink.prev = nullptr;

    lastLink.next = nullptr;
    lastLink.prev = &firstLink;
}

void LoadedSegment::AddEntity(Entity* entity)
{
    firstLink.InsertAfterThis(entity);
}

void LoadedSegment::BroadcastEvent(const IEntityEvent& ev)
{
    for (auto node = firstLink.next; node != &lastLink; node = node->next)
    {
        node->GetEntity()->SendEvent(ev);
    }
}

Scene::Scene(Engine* engine, StringId mapSegmentName)
    : _mapSegmentName(mapSegmentName),
    _cameraFollower(&_camera, engine->GetInput()),
    _engine(engine),
    _freeEntityHeaders(_entityHeaders.begin(), MaxEntities),
    _world(std::make_unique<b2World>(b2Vec2(0, 0))),
    _collisionManager(_world.get()),
    _freeSegments(_segmentPool, MaxLoadedSegments),
    replicationManager(this, engine->GetNetworkManger() != nullptr ? engine->GetNetworkManger()->IsServer() : false)
{
    for (int i = 0; i < MaxEntities; ++i)
    {
        _entityHeaders[i].id = InvalidEntityHeaderId;
    }

    _world->SetContactListener(&_collisionManager);

    _camera.SetScreenSize(engine->GetSdlManager()->WindowSize().AsVectorOfType<float>());
}

Scene::~Scene()
{
    DestroyScheduledEntities();

    for (int i = _entities.Size() - 1; i >= 0; --i)
    {
        DestroyEntity(_entities[i]);
    }

    DestroyScheduledEntities();
}

void Scene::RegisterEntity(Entity* entity, const EntityDictionary& properties)
{
    entity->_dimensions = properties.GetValueOrDefault<Vector2>("dimensions", Vector2(0, 0));
    entity->_rotation = properties.GetValueOrDefault<float>("rotation", 0);

    std::string_view name = properties.GetValueOrDefault<std::string_view>("name", "");
    entity->name = StringId(name);

    _entities.PushBackUnique(entity);

    AddIfImplementsInterface(_updatables, entity);
    AddIfImplementsInterface(_fixedUpdatables, entity);
    AddIfImplementsInterface(_renderables, entity);
    AddIfImplementsInterface(_hudRenderables, entity);

    EntityHeader* header = _freeEntityHeaders.Borrow();

    header->id = _nextEntityId++;
    header->entity = entity;

    entity->id = header->id;
    entity->header = header;
    entity->scene = this;
    _engine->GetSoundManager()->AddSoundEmitter(&entity->_soundEmitter, entity);

    // Make sure entities created in OnAdded() don't get offset by the segment offset
    auto oldEntityOffset = _entityOffset;
    _entityOffset = Vector2::Zero();
    entity->OnAdded(properties);

    _entityOffset = oldEntityOffset;
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

    RemoveIfImplementsInterface(_updatables, entity);
    RemoveIfImplementsInterface(_fixedUpdatables, entity);
    RemoveIfImplementsInterface(_renderables, entity);
    RemoveIfImplementsInterface(_hudRenderables, entity);

    EntityHeader* header = entity->header;
    header->id = InvalidEntityHeaderId;
    _freeEntityHeaders.Return(header);
    entity->SegmentLink::Unlink();

    auto memoryBlock = entity->GetMemoryBlock();
    entity->~Entity();
    _entities.RemoveSingle(entity);
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

    _toBeDestroyed.PushBack(entity);
}

void Scene::DestroyScheduledEntities()
{
    while (_toBeDestroyed.Size() != 0)
    {
        auto entityToDestroy = _toBeDestroyed[_toBeDestroyed.Size() - 1];
        _toBeDestroyed.PopBack();
        RemoveEntity(entityToDestroy);
    }
}

LoadedSegment* Scene::RegisterSegment(int segmentId)
{
    if (_loadedSegments.Size() == _loadedSegments.Capacity())
    {
        FatalError("Too many segments loaded");
    }

    LoadedSegment* segment = _freeSegments.Borrow();
    _loadedSegments.PushBack(segment);

    segment->Reset(segmentId);

    return segment;
}


LoadedSegment* Scene::GetLoadedSegment(int segmentId)
{
    for (auto segment : _loadedSegments)
    {
        if (segment->segmentId == segmentId)
        {
            return segment;
        }
    }

    return nullptr;
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

    for (auto renderable : _renderables)
    {
        renderable->Render(renderer);
    }

    // TODO: more efficient way of rendering components
    for(auto entity : _entities)
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
    for (auto hudRenderable : _hudRenderables)
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
    for (auto entity : _entities)
    {
        entity->SendEvent(ev);
    }

    SendEvent(ev);
}

SoundManager* Scene::GetSoundManager() const
{
    return _engine->GetSoundManager();
}

int Scene::LoadMapSegment(StringId id)
{
    auto mapSegment = ResourceManager::GetResource<MapSegment>(id);

    segmentsAdded.push_back(id);

    return LoadMapSegment(*mapSegment.Value());
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

int Scene::LoadMapSegment(const MapSegment& segment)
{
    auto loadedSegment = RegisterSegment(_currentSegmentId);

    auto oldEntityOffset = _entityOffset;

    if (_segmentOffset == Vector2(0, 0))
    {
        // Make the coordinates for the editor light up if the first segment so we don't have to worry
        // about segment offsets for the lights
        _segmentOffset = segment.startMarker;
    }

    auto offset = _segmentOffset - segment.startMarker;
    _entityOffset = offset;

    EntityDictionary tilemapProperties
    {
        { "type", "tilemap"_sid }
    };

    auto tileMap = CreateEntityInternal<TilemapEntity>(tilemapProperties);
    tileMap->SetMapSegment(segment);
    tileMap->segmentId = _currentSegmentId;
    tileMap->segmentStart = offset;
    loadedSegment->AddEntity(tileMap);

    loadedSegment->properties = segment.properties;

    EntityDictionary ambientLightProperties
    {
        { "type", "ambient-light"_sid }
    };

    AmbientLightEntity* ambientLight = nullptr;

    for (auto& instance : segment.entities)
    {
        EntityDictionary properties(instance.properties.get(), instance.totalProperties);
        auto entity = CreateEntity(properties);
        entity->segmentId = _currentSegmentId;
        loadedSegment->AddEntity(entity);

        if (entity->Is<AmbientLightEntity>())
        {
            ambientLight = static_cast<AmbientLightEntity*>(entity);
        }

        if (entity->flags & PreventUnloading)
        {
            entity->Unlink();
        }
    }

    if (ambientLight == nullptr)
    {
        ambientLight = CreateEntityInternal<AmbientLightEntity>(ambientLightProperties);
        loadedSegment->AddEntity(ambientLight);

        ambientLight->ambientLight.intensity = 1;
        ambientLight->ambientLight.color = Color(255, 255, 255);
        ambientLight->segmentId = _currentSegmentId;
    }

    Rectangle segmentBounds(Vector2(_segmentOffset.x, _segmentOffset.y - 1e5),
        Vector2((segment.endMarker - segment.startMarker).x, 2 * 1e5));

    ambientLight->ambientLight.bounds = segmentBounds;

    auto useLocalAmbientLight = segment.properties.GetValueOrDefault("useLocalAmbientLight", false);
    if (!useLocalAmbientLight)
    {
        ambientLight->ambientLight.intensity = currentAmbientBrightness;
    }

    _segmentOffset += segment.endMarker - segment.startMarker;

    _entityOffset = oldEntityOffset;

    auto segmentLoadedEvent = SegmentLoadedEvent(_currentSegmentId, segmentBounds, currentDifficulty);

    loadedSegment->BroadcastEvent(segmentLoadedEvent);
    SendEvent(segmentLoadedEvent);

    return _currentSegmentId++;
}

void Scene::UnloadSegmentsBefore(int segmentId)
{
    int lastAliveIndex = 0;

    for (int i = 0; i < _loadedSegments.Size(); ++i)
    {
        auto segment = _loadedSegments[i];

        if (segment->segmentId < segmentId && segment->segmentId >= 0)
        {
            for (auto node = segment->firstLink.next; node != &segment->lastLink; node = node->next)
            {
                node->GetEntity()->Destroy();
            }

            segment->Reset(-2);
            _freeSegments.Return(segment);
        }
        else
        {
            _loadedSegments[lastAliveIndex++] = segment;
        }
    }

    _loadedSegments.Resize(lastAliveIndex);
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

bool Scene::RaycastExcludingSelf(Vector2 start, Vector2 end, Entity* self, RaycastResult& outResult, bool allowTriggers, const std::function<bool(ColliderHandle handle)>& includeFixture)
{
    FindClosestRaycastCallback callback(self, allowTriggers, includeFixture);

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

std::vector<Entity*> Scene::GetEntitiesByNameInSegment(StringId name, int segmentId)
{
    std::vector<Entity*> foundEntities;

    auto segment = GetLoadedSegment(segmentId);

    if (segment == nullptr)
    {
        return { };
    }

    for (auto node = segment->firstLink.next; node != &segment->lastLink; node = node->next)
    {
        auto entity = node->GetEntity();

        if (entity->name == name)
        {
            foundEntities.push_back(entity);
        }
    }

    return foundEntities;
}

Entity* Scene::GetFirstEntityByNameInSegment(StringId name, int segmentId)
{
    auto segment = GetLoadedSegment(segmentId);

    if (segment == nullptr)
    {
        return { };
    }

    for (auto node = segment->firstLink.next; node != &segment->lastLink; node = node->next)
    {
        auto entity = node->GetEntity();

        if (entity->name == name)
        {
            return entity;
        }
    }

    return nullptr;
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
        for (auto fixedUpdatable : _fixedUpdatables)
        {
            fixedUpdatable->FixedUpdate(PhysicsDeltaTime);
        }

        SendEvent(FixedUpdateEvent());

        _world->Step(PhysicsDeltaTime, 8, 3);

        _collisionManager.UpdateEntityPositions();
    }

    PreUpdate();
    SendEvent(PreUpdateEvent());

    for (auto updatable : _updatables)
    {
        updatable->Update(deltaTime);
    }

    _timerManager.TickTimers(deltaTime);

    PostUpdate();


    DestroyScheduledEntities();

    _cameraFollower.Update(deltaTime);
}

bool Scene::TryGetLoadedSegmentById(const int segmentId, LoadedSegment*& segment)
{
    for (int i = 0; i < _loadedSegments.Size(); ++i)
    {
        if (_loadedSegments[i]->segmentId == segmentId)
        {
            segment = _loadedSegments[i];
            return true;
        }
    }

    return false;
}