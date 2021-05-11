#include "EntityManager.hpp"

EntityManager::EntityManager()
    : freeEntityHeaders(entityHeaders.begin(), MaxEntities)
{
    for (int i = 0; i < MaxEntities; ++i)
    {
        entityHeaders[i].id = InvalidEntityHeaderId;
    }
}

void EntityManager::RegisterEntity(Entity* entity)
{
    entities.insert(entity);

    EntityHeader* header = freeEntityHeaders.Borrow();

    header->id = _nextEntityId++;
    header->entity = entity;

    entity->id = header->id;
    entity->header = header;

    // Put into entity group
    {
        auto groupIt = entitiesByType.find(entity->type.key);

        if (groupIt == entitiesByType.end())
        {
            // We don't know whether the entity is updatable, renderable, etc since there's no way to introspect the vtable
            // to see if the methods are overridden. So, assume it implements those things and let the base class implementation
            // turn them off for the group.
            auto[it, inserted] = entitiesByType.try_emplace(entity->type.key);
            groupIt = it;

            EntityGroup* group = &it->second;

            updatables.insert(group);
            fixedUpdatables.insert(group);
            renderables.insert(group);

            serverUpdatables.insert(group);
            serverFixedUpdatables.insert(group);
        }

        groupIt->second.entities.Append(entity);
        entity->entityGroup = &groupIt->second;
    }
}

void EntityManager::UnregisterEntity(Entity* entity)
{
    entities.erase(entity);
    entity->Unlink();

    EntityHeader* header = entity->header;
    header->id = InvalidEntityHeaderId;
    freeEntityHeaders.Return(header);
}

void EntityManager::RunHookRemovals()
{
    for (auto& removal : scheduledHookRemovals)
    {
        removal.hookSet->erase(removal.entityGroup);
    }

    scheduledHookRemovals.clear();
}
