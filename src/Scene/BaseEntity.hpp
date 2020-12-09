#pragma once

#include "Entity.hpp"
#include "Scene.hpp"

template<typename TEntity>
constexpr const char* EntityName = "unknown";

/// <summary>
/// Defines a new entity.
/// </summary>
#define DEFINE_ENTITY(structName_, stringIdName_)           \
struct structName_; \
template<>                                                  \
static constexpr const char* EntityName<structName_> = stringIdName_;           \
struct structName_ : BaseEntity<structName_>

namespace EntityUtil
{
    struct EntityMetadata
    {
        EntityMetadata(StringId type_, Entity* (*createEntity_)(Scene* scene, const EntityDictionary& properties))
            : type(type_),
            createEntity(createEntity_)
        {
            Register(this);
        }

        static Entity* CreateEntityFromType(Scene* scene, const EntityDictionary& properties);
        static void Register(EntityMetadata* metadata);

        StringId type;
        Entity* (*createEntity)(Scene* scene, const EntityDictionary& properties);
    };
}

template<typename TDerived>
struct BaseEntity : Entity
{
    static constexpr StringId Type = StringId(EntityName<TDerived>);

    virtual EntityUtil::EntityMetadata* GetMetadata() { return &metadata; }

    static Entity* CreateEntity(Scene* scene, const EntityDictionary& properties)
    {
        return scene->CreateEntityInternal<TDerived>(properties);
    }

    static EntityUtil::EntityMetadata metadata;

private:
    std::pair<int, void*> GetMemoryBlock() override
    {
        return std::make_pair<int, void*>(sizeof(TDerived), static_cast<TDerived*>(this));
    }
};

template<typename TDerived>
EntityUtil::EntityMetadata BaseEntity<TDerived>::metadata(Type, CreateEntity);
