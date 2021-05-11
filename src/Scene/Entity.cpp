#include "Entity.hpp"
#include "BaseEntity.hpp"
#include "Engine.hpp"
#include "Scene.hpp"
#include "IEntityEvent.hpp"

#include <unordered_map>

const EntityHeader InvalidEntityHeader::InvalidHeader;

void* AllocateComponent(Scene* scene, int size)
{
    return scene->AllocateMemory(size);
}

std::unordered_map<unsigned, EntityUtil::EntityMetadata*>& GetEntityMetadataByType()
{
    static std::unordered_map<unsigned int, EntityUtil::EntityMetadata*> metadataByType;
    return metadataByType;
}

Entity* EntityUtil::EntityMetadata::CreateEntityFromType(StringId type, Scene* scene, EntitySerializer& serializer)
{
    auto& entityTypes = GetEntityMetadataByType();

    auto it = entityTypes.find(type);
    if (it == entityTypes.end())
    {
        FatalError("No entity type registered with id %u", type.key);
    }

    return it->second->createEntity(scene, serializer);
}

void EntityUtil::EntityMetadata::Register(EntityMetadata* metadata)
{
    GetEntityMetadataByType()[metadata->type.key] = metadata;
}

Entity::~Entity()
{
    Entity* child = children;
    while (child != nullptr)
    {
        auto next = child->nextSibling;
        child->nextSibling = nullptr;
        child->parent = nullptr;

        child = next;
    }

    if (parent != nullptr)
    {
        if (parent->children == this)
        {
            parent->children = nextSibling;
        }
        else
        {
            auto node = parent->children;

            while (node != nullptr)
            {
                auto next = node->nextSibling;
                if (next == this)
                {
                    node->nextSibling = next->nextSibling;
                    break;
                }

                node = next;
            }
        }
    }
}

void Entity::Destroy()
{
    scene->MarkEntityForDestruction(this);
}

void Entity::SetCenter(const Vector2& newPosition)
{
    bool moved = _position.Value() != newPosition;

    if (moved)
    {
        _position = newPosition;
        DoTeleport();
    }
}

void Entity::DoTeleport()
{
    NotifyMovement();
    flags.SetFlag(EntityFlags::WasTeleported);
}


void Entity::SetRotation(float angle)
{
    _rotation = angle;
    NotifyMovement();
}

Engine* Entity::GetEngine() const
{
    return scene->GetEngine();
}

void Entity::SendEvent(const IEntityEvent& ev)
{
    if (scene->isServer)
    {
        ReceiveServerEvent(ev);
    }
    else
    {
        ReceiveEvent(ev);
    }

    for (auto component = _componentList; component != nullptr; component = component->next)
    {
        component->ReceiveEvent(ev);
    }
}

void Entity::Serialize(EntitySerializer& serializer)
{
    // TODO: add position and rotation
    DoSerialize(serializer);
}

void Entity::StartTimer(float timeSeconds, const std::function<void()>& callback)
{
    scene->StartEntityTimer(timeSeconds, callback, this);
}

void Entity::AddChild(Entity* child)
{
    child->parent = this;
    child->nextSibling = children;
    children = child;
}

SoundChannel* Entity::GetChannel(int id)
{
    Assert(id >= 0 && id < SoundEmitter::MaxChannels);

    return _soundEmitter.channels + id;
}

void Entity::UpdateSyncVars()
{
    if (_position.Changed())
    {
        DoTeleport();
    }

    OnSyncVarsUpdated();
}

void Entity::RemoveComponent(IEntityComponent* component)
{
    if (component == _componentList)
    {
        _componentList = _componentList->next;
    }
    else
    {
        for (auto c = _componentList; c != nullptr; c = c->next)
        {
            if (c->next == component)
            {
                c->next = component->next;
                return;
            }
        }

        FatalError("Component %s is not in component list of %s",
            typeid(*component).name(),
            typeid(*this).name());
    }
}

void Entity::NotifyMovement()
{
    SendEvent(EntityMovedEvent());

    for (auto component = _componentList; component != nullptr; component = component->next)
    {
        component->OnOwnerMoved();
    }
}

Entity::Entity()
{

}

template<typename THookSelector>
static void DisableHook(Entity* entity, THookSelector selector)
{
    auto& entityManager = entity->scene->GetEntityManager();
    auto set = selector(entityManager);
    entityManager.scheduledHookRemovals.emplace_back(set, entity->entityGroup);
}

void Entity::Update(float deltaTime)
{
    DisableHook(this, [](auto& em) { return &em.updatables; });
}

void Entity::ServerUpdate(float deltaTime)
{
    DisableHook(this, [](auto& em) { return &em.serverUpdatables; });
}

void Entity::FixedUpdate(float deltaTime)
{
    DisableHook(this, [](auto& em) { return &em.fixedUpdatables; });
}

void Entity::ServerFixedUpdate(float deltaTime)
{
    DisableHook(this, [](auto& em) { return &em.serverFixedUpdatables; });
}

void Entity::Render(Renderer* renderer)
{
    DisableHook(this, [](auto& em) { return &em.renderables; });
}

Vector2 Entity::ScreenCenter() const
{
    auto input = Center();

    auto output = scene->isometricSettings.TileToScreenIncludingTerrain(
        scene->isometricSettings.WorldToTile(Center()));

    return output;
}
