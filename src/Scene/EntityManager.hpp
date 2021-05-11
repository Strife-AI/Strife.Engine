#pragma once
#include <unordered_set>
#include <vector>
#include <robin_hood.h>

#include "Entity.hpp"
#include "Memory/FreeList.hpp"

constexpr int MaxEntities = 8192;

template<typename TEntity>
struct EntityListIterator
{
public:
    EntityListIterator(const DLinkedListIterator<Entity>& current)
        : _current(current)
    {

    }

    TEntity* operator*()
    {
        return static_cast<TEntity*>(*_current);
    }

    EntityListIterator operator++()
    {
        return EntityListIterator(++_current);
    }

    bool operator==(const EntityListIterator& rhs) const
    {
        return _current == rhs._current;
    }

    bool operator!=(const EntityListIterator& rhs) const
    {
        return !(*this == rhs);
    }

private:
    DLinkedListIterator<Entity> _current;
};

template<typename TEntity>
struct EntityList
{
    EntityList(DLinkedList<Entity>* list)
        : _list(list)
    {

    }

    EntityListIterator<TEntity> begin()
    {
        return EntityListIterator<TEntity>(_list->begin());
    }

    EntityListIterator<TEntity> end()
    {
        return EntityListIterator<TEntity>(_list->end());
    }

private:
    DLinkedList<Entity>* _list;
};

struct EntityGroup
{
    DLinkedList<Entity> entities;
};

struct ScheduledHookRemoval
{
    ScheduledHookRemoval(robin_hood::unordered_flat_set<EntityGroup*>* hookSet, EntityGroup* entityGroup)
        : hookSet(hookSet),
          entityGroup(entityGroup)
    {

    }

    robin_hood::unordered_flat_set<EntityGroup*>* hookSet;
    EntityGroup* entityGroup;
};

struct EntityManager
{
    static constexpr int InvalidEntityHeaderId = -2;

    EntityManager();
    EntityManager(const EntityManager& rhs) = delete;

    void RegisterEntity(Entity* entity);
    void UnregisterEntity(Entity* entity);
    void RunHookRemovals();

    template<typename TEntity>
    EntityList<TEntity> GetEntitiesOfType();

    FreeList<EntityHeader> freeEntityHeaders;

    robin_hood::unordered_map<unsigned int, EntityGroup> entitiesByType;

    FixedSizeVector<EntityHeader, MaxEntities> entityHeaders;

	robin_hood::unordered_flat_set<Entity*> entities;
    robin_hood::unordered_flat_set<EntityGroup*> updatables;
    robin_hood::unordered_flat_set<EntityGroup*> serverUpdatables;
    robin_hood::unordered_flat_set<EntityGroup*> fixedUpdatables;
    robin_hood::unordered_flat_set<EntityGroup*> serverFixedUpdatables;
    robin_hood::unordered_flat_set<EntityGroup*> renderables;

    std::vector<Entity*> toBeDestroyed;

    int _nextEntityId = 1;

    DLinkedList<Entity> emptyEntityList;
    std::vector<ScheduledHookRemoval> scheduledHookRemovals;
};

template<typename TEntity>
EntityList<TEntity> EntityManager::GetEntitiesOfType()
{
    decltype(entitiesByType)::iterator entityGroup = entitiesByType.find(TEntity::Type.key);
    if (entityGroup == entitiesByType.end())
    {
        return EntityList<TEntity>(&emptyEntityList);
    }
    else
    {
        return EntityList<TEntity>(&entityGroup->second.entities);
    }
}
