#include "Entity.hpp"
#include "BaseEntity.hpp"
#include "Scene.hpp"

const EntityHeader InvalidEntityHeader::InvalidHeader;

std::unordered_map<unsigned, EntityUtil::EntityMetadata*>& GetEntityMetadataByType()
{
    static std::unordered_map<unsigned int, EntityUtil::EntityMetadata*> metadataByType;
    return metadataByType;
}

Entity* EntityUtil::EntityMetadata::CreateEntityFromType(Scene* scene, const EntityDictionary& properties)
{
    auto& metadata = GetEntityMetadataByType();

    StringId key;

    std::string_view type;
    if (!properties.TryGetProperty("type", type))
    {
        for (int i = 0; i < properties.TotalProperties(); ++i)
        {
            printf("%s: %s\n", properties.Properties()[i].key.c_str(), properties.Properties()[i].value.c_str());
        }

        FatalError("Entity missing type property");
    }

    if (type[0] == '#')
    {
        // Type is given as a string id
        properties.TryGetProperty("type", key);
    }
    else
    {
        key = StringId(type);
    }

    auto it = metadata.find(key);

    if (it == metadata.end())
    {
        properties.Print();

        for (int i = 0; i < properties.TotalProperties(); ++i)
        {
            printf("%s: %s\n", properties.Properties()[i].key.c_str(), properties.Properties()[i].value.c_str());
        }

        std::string name(type.data(), type.data() + type.length());
        FatalError("Unknown entity type: %s\n", name.c_str());
    }

    return it->second->createEntity(scene, properties);
}

void EntityUtil::EntityMetadata::Register(EntityMetadata* metadata)
{
    GetEntityMetadataByType()[metadata->type.key] = metadata;
}

void EntityDictionary::Print() const
{
    printf("==================\n");
    for (int i = 0; i < _totalProperties; ++i)
    {
        printf("%s: %s\n", _properties[i].key.c_str(), _properties[i].value.c_str());
    }
}

EntityDictionaryBuilder& EntityDictionaryBuilder::AddProperty(const EntityProperty& property)
{
    for (auto& p : _properties)
    {
        if (p.key == property.key)
        {
            p = property;
            return *this;
        }
    }

    _properties.push_back(property);

    return *this;
}

EntityDictionaryBuilder& EntityDictionaryBuilder::AddMap(const std::map<std::string, std::string>& properties)
{
    for (const auto& pair : properties)
    {
        AddProperty({ pair.first.c_str(), pair.second.c_str() });
    }

    return *this;
}

std::map<std::string, std::string> EntityDictionaryBuilder::BuildMap()
{
    std::map<std::string, std::string> result;

    for (auto& p : _properties)
    {
        result[p.key.c_str()] = p.value.c_str();
    }

    return result;
}

Entity* SegmentLink::GetEntity()
{
    return static_cast<Entity*>(this);
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
    scene->DestroyEntity(this);
}

void Entity::SetCenter(const Vector2& newPosition)
{
    flags |= WasTeleported;
    bool moved = _position != newPosition;

    if (moved)
    {
        _position = newPosition;
        NotifyMovement();
    }
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

void Entity::Serialize(EntityDictionaryBuilder& builder)
{
    builder
        .AddProperty({ "position", TopLeft() })
        .AddProperty({ "rotation", Rotation() })
        .AddProperty({ "dimensions", Dimensions() })
        .AddProperty({ "type", type })
        .AddProperty({ "name", name });

    DoSerialize(builder);
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

void Entity::RemoveComponent(IEntityComponent* component)
{
    if (component == _componentList)
    {
        _componentList = _componentList->next;
    }
    else
    {
        for(auto c = _componentList; c != nullptr; c = c->next)
        {
            if(c->next == component)
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
