#pragma once

#include <functional>
#include <box2d/b2_body.h>
#include <map>
#include <string>

#ifdef _WIN32
#include <vcruntime_typeinfo.h>
#endif

#include <charconv>


#include "EntityComponent.hpp"
#include "Math/Vector2.hpp"
#include "Memory/Dictionary.hpp"
#include "Memory/DLinkNode.hpp"
#include "Memory/FixedLengthString.hpp"
#include "Physics/Physics.hpp"
#include "Memory/StringId.hpp"

#include "Renderer/Color.hpp"
#include "Sound/SoundManager.hpp"

class b2Body;
struct IEntityEvent;
struct MoveResult;
class Renderer;
class Scene;
struct Entity;
class Engine;
struct EntityInstance;

struct IRenderable { virtual void Render(Renderer* renderer) = 0; };
struct IHudRenderable { virtual void RenderHud(Renderer* renderer) = 0; };
struct IUpdatable { virtual void Update(float deltaTime) = 0; };
struct IFixedUpdatable { virtual void FixedUpdate(float deltaTime) = 0; };

struct EntityHeader
{
    EntityHeader()
    {
        
    }

    Entity* entity;
    int id;
};

enum EntityFlags
{
    EnableBuoyancy = 1,
    WasTeleported = 2,
    CastsShadows = 4,
    PreventUnloading = 8
};

struct EntityProperty
{
    EntityProperty() = default;

    constexpr EntityProperty(const char* key_, const char* value_)
        : key(key_),
        value(value_)
    {

    }

    EntityProperty(const char* key_, float value_)
        : key(key_)
    {
        value.VFormat("%f", value_);
    }

    EntityProperty(const char* key_, int value_)
        : key(key_)
    {
        value.VFormat("%d", value_);
    }

    EntityProperty(const char* key_, StringId value_)
        : key(key_)
    {
        value.VFormat("#%u", value_.key);
    }

    EntityProperty(const char* key_, Vector2 value_)
        : key(key_)
    {
        value.VFormat("%f %f", value_.x, value_.y);
    }

    EntityProperty(const char* key_, Color color)
        : key(key_)
    {
        value.VFormat("%d %d %d %d", color.r, color.g, color.b, color.a);
    }

    template<typename TEntity>
    static constexpr EntityProperty EntityType()
    {
        return EntityProperty("type", TEntity::Type);
    }

    FixedLengthString<32> key;
    FixedLengthString<128> value;
};

class EntityDictionary : public Dictionary<EntityDictionary>
{
public:
    int TotalProperties() const { return _totalProperties; }
    const EntityProperty* Properties() const { return _properties; }

    constexpr EntityDictionary(EntityProperty* properties, int count)
        : _totalProperties(count),
        _properties(properties)
    {

    }

    constexpr EntityDictionary()
        : _totalProperties(0),
        _properties(nullptr)
    {

    }

    constexpr EntityDictionary(const std::initializer_list<EntityProperty>& properties)
        : _totalProperties(properties.size()),
        _properties(&properties.begin()[0])
    {

    }

    const EntityProperty* begin() const
    {
        return _properties;
    }

    const EntityProperty* end() const
    {
        return _properties + _totalProperties;
    }

    void Print() const;

private:
    template<typename T>
    bool TryGetCustomProperty(const char* key, T& outResult) const
    {
        return false;
    }

    bool TryGetStringViewProperty(const char* key, std::string_view& result) const
    {
        for (int i = 0; i < _totalProperties; ++i)
        {
            if (_properties[i].key == key)
            {
                result = std::string_view(_properties[i].value.c_str(), _properties[i].value.Length());
                return true;
            }
        }

        return false;
    }

    int _totalProperties = 0;
    const EntityProperty* _properties;
    friend class Dictionary<EntityDictionary>;
};

class EntityDictionaryBuilder
{
public:
    EntityDictionaryBuilder& AddProperty(const EntityProperty& property);
    EntityDictionaryBuilder& AddMap(const std::map<std::string, std::string>& properties);

    EntityDictionary Build()
    {
        return EntityDictionary(&_properties[0], _properties.size());
    }

    std::map<std::string, std::string> BuildMap();
    std::vector<EntityProperty> BuildList() const { return _properties; }

private:
    std::vector<EntityProperty> _properties;
};

struct Entity;

struct SegmentLink : DLinkNode<SegmentLink>
{
    Entity* GetEntity();
};

/// <summary>
/// The base class of all entities. Do not inherit from this directly. Instead, use the macro <see cref="DEFINE_ENTITY"/>
/// </summary>
struct Entity : SegmentLink
{
    static const int InvalidEntityId = -1;

    Entity(const Entity&) = delete;
    Entity() = default;
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

    Vector2 TopLeft() const { return _position - _dimensions / 2; }
    Vector2 Center() const { return _position; }
    Vector2 Dimensions() const { return _dimensions; }
    Rectangle Bounds() const { return Rectangle(TopLeft(), Dimensions()); }
    float Rotation() const { return _rotation; }
    Engine* GetEngine() const;
    const char* DebugName() const { return typeid(*this).name(); }

    /// <summary>
    /// Called when an entity has been added to the scene.
    /// </summary>
    /// <param name="properties">A dictionary of serialized entity properties</param>
    virtual void OnAdded(const EntityDictionary& properties) { }

    /// <summary>
    /// Called when an entity is about to be destroyed and removed from the scene.
    /// </summary>
    virtual void OnDestroyed() { }

    /// <summary>
    /// Sends an event directly to an entity.
    /// </summary>
    /// <param name="ev"></param>
    void SendEvent(const IEntityEvent& ev) { OnEvent(ev); }

    void Serialize(EntityDictionaryBuilder& builder);

    /// <summary>
    /// Checks if an entity is a given type.
    /// </summary>
    /// <typeparam name="TEntity"></typeparam>
    /// <returns></returns>
    template<typename TEntity> bool Is() { return type == TEntity::Type; }

    /// <summary>
    /// Checks if an entity is a given type. If so, it will write the pointer to outEntity. Otherwise, outEntity will be nullptr.
    /// </summary>
    /// <typeparam name="TEntity"></typeparam>
    /// <param name="outEntity"></param>
    /// <returns></returns>
    template<typename TEntity> bool Is(TEntity*& outEntity);

    /// <summary>
    /// Starts a timer that is attached to an entity. If the entity is destroyed before the time expires, the timer
    /// is aborted.
    /// </summary>
    /// <param name="timeSeconds"></param>
    /// <param name="callback"></param>
    void StartTimer(float timeSeconds, const std::function<void()>& callback);
    void AddChild(Entity* child);

    SoundChannel* GetChannel(int id);

    template<typename TComponent, typename ...Args> TComponent* AddComponent(const char* name = "<default>", Args&& ...args);
    template<typename TComponent> TComponent* GetComponent();
    void RemoveComponent(IEntityComponent* component);

    int id;
    StringId name;
    StringId type;
    EntityHeader* header;
    bool isDestroyed = false;
    Scene* scene;
    int observedObjectType = 0;

    unsigned int flags = 0;
    int segmentId;
    Entity* parent = nullptr;
    Entity* nextSibling = nullptr;
    Entity* children = nullptr;

protected:
    void NotifyMovement();

private:
    friend class Scene;
    friend void MoveEntityRecursive(RigidBodyComponent* rigidBody, Vector2 offset);

    virtual void OnEvent(const IEntityEvent& ev) { }
    virtual void DoSerialize(EntityDictionaryBuilder& writer) { }
    virtual std::pair<int, void*> GetMemoryBlock() = 0;

    Vector2 _position;
    Vector2 _dimensions;
    float _rotation;
    int _lastQueryId = -1;
    SoundEmitter _soundEmitter;

    IEntityComponent* _componentList = nullptr;
};

template <typename TEntity>
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

template <typename TComponent, typename ...Args>
TComponent* Entity::AddComponent(const char* name, Args&& ...args)
{
    auto newComponent = static_cast<TComponent*>(scene->AllocateMemory(sizeof(TComponent)));

    new (newComponent) TComponent(std::forward<Args>(args)...);
    newComponent->owner = this;
    newComponent->name = name;

    newComponent->next = _componentList;
    _componentList = newComponent;

    newComponent->OnAdded();

    return newComponent;
}

template <typename TComponent>
TComponent* Entity::GetComponent()
{
    for(auto component = _componentList; component != nullptr; component = component->next)
    {
        if(auto c = component->Is<TComponent>())
        {
            return c;
        }
    }

    FatalError("Missing component %s on type %s", typeid(TComponent).name(), typeid(*this).name());
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
        if(entity == nullptr)
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
        return {&InvalidEntityHeader::InvalidHeader, Entity::InvalidEntityId};
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

