#pragma once

#include <functional>
#include <map>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <vcruntime_typeinfo.h>
#endif

#include "EntityComponent.hpp"
#include "Math/Vector2.hpp"
#include "Physics/Physics.hpp"
#include "Memory/StringId.hpp"
#include "Net/SyncVar.hpp"

#include "Sound/SoundManager.hpp"
#include "EntitySerializer.hpp"
#include "Memory/DLinkNode.hpp"

class b2Body;
struct IEntityEvent;
struct MoveResult;
class Renderer;
class Scene;
struct Entity;
class Engine;
struct EntityInstance;
struct EntityGroup;

struct EntityHeader
{
    EntityHeader()
    {

    }

    Entity* entity;
    int id;
};

enum class EntityFlags
{
    EnableBuoyancy = 1,
    WasTeleported = 2,
    CastsShadows = 4,
};

struct Entity;
struct ISyncVar;

/// <summary>
/// The base class of all entities. Do not inherit from this directly. Instead, use the macro <see cref="DEFINE_ENTITY"/>
/// </summary>
struct Entity : DLinkedNode<Entity>
{
    static const int InvalidEntityId = -1;

    Entity(const Entity&) = delete;
    Entity();
    virtual ~Entity();

    /// <summary>
    /// Schedules an entity to be destroyed.
    /// </summary>
    void Destroy();

    /// <summary>
    /// Sets the center position of this entity. This will trigger an EntityMovedEvent() and notify all components
    /// that the owning entity moved.
    /// </summary>
    /// <param name="newPosition"></param>
    void SetCenter(const Vector2& newPosition);
    void SetRotation(float angle);

    Vector2 TopLeft() const
    {
        return _position.Value() - _dimensions / 2;
    }

    Vector2 Center() const
    {
        return _position.Value();
    }

    Vector2 ScreenCenter() const;

    Vector2 Dimensions() const
    {
        return _dimensions;
    }

    void SetDimensions(Vector2 dimensions)
    {
        _dimensions = dimensions;
    }

    Rectangle Bounds() const
    {
        return Rectangle(TopLeft(), Dimensions());
    }

    float Rotation() const
    {
        return _rotation;
    }

    Engine* GetEngine() const;

    const char* DebugName() const
    {
        return typeid(*this).name();
    }

    virtual void Update(float deltaTime);
    virtual void ServerUpdate(float deltaTime);

    virtual void FixedUpdate(float deltaTime);
    virtual void ServerFixedUpdate(float deltaTime);

    virtual void Render(Renderer* renderer);

    /// <summary>
    /// Called when an entity has been added to the scene.
    /// </summary>
    virtual void OnAdded()
    {
    }

    /// <summary>
    /// Called when an entity is about to be destroyed and removed from the scene.
    /// </summary>
    virtual void OnDestroyed()
    {
    }

    /// <summary>
    /// Sends an event directly to an entity.
    /// </summary>
    /// <param name="ev"></param>
    void SendEvent(const IEntityEvent& ev);

    void Serialize(EntitySerializer& serializer);

    /// <summary>
    /// Checks if an entity is a given type.
    /// </summary>
    /// <typeparam name="TEntity"></typeparam>
    /// <returns></returns>
    template<typename TEntity>
    bool Is()
    {
        return type == TEntity::Type;
    }

    /// <summary>
    /// Checks if an entity is a given type. If so, it will write the pointer to outEntity. Otherwise, outEntity will be nullptr.
    /// </summary>
    /// <typeparam name="TEntity"></typeparam>
    /// <param name="outEntity"></param>
    /// <returns></returns>
    template<typename TEntity>
    bool Is(TEntity*& outEntity);

    /// <summary>
    /// Starts a timer that is attached to an entity. If the entity is destroyed before the time expires, the timer
    /// is aborted.
    /// </summary>
    /// <param name="timeSeconds"></param>
    /// <param name="callback"></param>
    void StartTimer(float timeSeconds, const std::function<void()>& callback);
    void AddChild(Entity* child);

    SoundChannel* GetChannel(int id);

    void UpdateSyncVars();

    template<typename TComponent, typename ...Args>
    TComponent* AddComponent(Args&& ...args);
    template<typename TComponent>
    TComponent* GetComponent(bool fatalIfMissing = true);
    template<typename TComponent>
    bool TryGetComponent(TComponent*& component);

    void RemoveComponent(IEntityComponent* component);

    int id;
    StringId name;
    StringId type;
    EntityHeader* header;
    bool isDestroyed = false;
    Scene* scene;

    Flags<EntityFlags> flags;
    Entity* parent = nullptr;
    Entity* nextSibling = nullptr;
    Entity* children = nullptr;
    EntityGroup* entityGroup;

    ISyncVar* syncVarHead = nullptr;

    long long lastQueryId = -1;

protected:
    void NotifyMovement();

private:
    friend class Scene;

    friend void MoveEntityRecursive(RigidBodyComponent* rigidBody, Vector2 offset);

    virtual void ReceiveEvent(const IEntityEvent& ev)
    {

    }

    virtual void ReceiveServerEvent(const IEntityEvent& ev)
    {

    }

    void DoTeleport();
    void FlagsChanged();

    virtual void DoSerialize(EntitySerializer& serializer)
    {

    }

    virtual std::pair<int, void*> GetMemoryBlock() = 0;

    virtual void OnSyncVarsUpdated()
    {

    }

    SyncVar<Vector2> _position{{ 0, 0 }, SyncVarInterpolation::Linear, SyncVarUpdateFrequency::Frequent,
                               SyncVarDeltaMode::Full };

    Vector2 _dimensions;
    float _rotation = 0;
    SoundEmitter _soundEmitter;

    IEntityComponent* _componentList = nullptr;
};

template<typename TEntity>
bool Entity::Is(TEntity*& outEntity)
{
    if (type == TEntity::Type)
    {
        outEntity = static_cast<TEntity*>(this);
        return true;
    }
    else
    {
        outEntity = nullptr;
        return false;
    }
}

void* AllocateComponent(Scene* scene, int size);

template<typename TComponent, typename ...Args>
TComponent* Entity::AddComponent(Args&& ...args)
{
    auto newComponent = static_cast<TComponent*>(AllocateComponent(scene, sizeof(TComponent)));

    new(newComponent) TComponent(std::forward<Args>(args)...);
    newComponent->owner = this;

    newComponent->next = _componentList;
    _componentList = newComponent;

    newComponent->Register();
    newComponent->OnAdded();

    return newComponent;
}

template<typename TComponent>
TComponent* Entity::GetComponent(bool fatalIfMissing)
{
    for (auto component = _componentList; component != nullptr; component = component->next)
    {
        if (auto c = component->Is<TComponent>())
        {
            return c;
        }
    }

    if (fatalIfMissing)
    {
        FatalError("Missing component %s on type %s", typeid(TComponent).name(), typeid(*this).name());
    }
    else
    {
        return nullptr;
    }
}

template<typename TComponent>
bool Entity::TryGetComponent(TComponent*& out)
{
    if (this->isDestroyed) return false;

    TComponent* component = GetComponent<TComponent>(false);

    if (component != nullptr)
    {
        out = component;
        return true;
    }

    out = nullptr;
    return false;
}


struct InvalidEntityHeader
{
    static const EntityHeader InvalidHeader;
};

template<typename TEntity>
class EntityReference
{
public:
    EntityReference()
        : _entityHeader(&InvalidEntityHeader::InvalidHeader),
          _entityId(Entity::InvalidEntityId)
    {
    }

    EntityReference(const EntityReference&) = default;

    explicit EntityReference(TEntity* entity)
        : _entityHeader(entity->header),
          _entityId(entity->id)
    {

    }

    EntityReference& operator=(const EntityReference&) = default;

    EntityReference& operator=(TEntity* entity)
    {
        if (entity == nullptr)
        {
            Invalidate();

            return *this;
        }
        else
        {
            _entityHeader = entity->header;
            _entityId = entity->id;

            return *this;
        }
    }

    bool operator==(const EntityReference& rhs) const
    {
        return _entityHeader == rhs._entityHeader && _entityId == rhs._entityId;
    }

    bool operator==(const TEntity* rhs) const
    {
        return _entityHeader->entity == rhs;
    }

    bool IsValid() const
    {
        return _entityHeader->id == _entityId;
    }

    bool TryGetValue(TEntity*& entity) const
    {
        if (IsValid())
        {
            entity = static_cast<TEntity*>(_entityHeader->entity);
            return true;
        }
        else
        {
            entity = nullptr;
            return false;
        }
    }

    void Invalidate()
    {
        _entityHeader = &InvalidEntityHeader::InvalidHeader;
        _entityId = Entity::InvalidEntityId;
    }

    TEntity* GetValueOrNull() const
    {
        TEntity* entity;
        return TryGetValue(entity) ? entity : nullptr;
    }

    static EntityReference Invalid()
    {
        return { &InvalidEntityHeader::InvalidHeader, Entity::InvalidEntityId };
    }

private:
    EntityReference(const EntityHeader* entityHeader, int entityId)
        : _entityHeader(entityHeader),
          _entityId(entityId)
    {
    }

    const EntityHeader* _entityHeader;
    int _entityId;
};
