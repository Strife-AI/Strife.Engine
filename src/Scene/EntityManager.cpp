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
}

void EntityManager::UnregisterEntity(Entity* entity)
{
    entities.erase(entity);

    updatables.erase(entity);
    fixedUpdatables.erase(entity);
    serverUpdatables.erase(entity);
    serverFixedUpdatables.erase(entity);
    renderables.erase(entity);
    hudRenderables.erase(entity);

    EntityHeader* header = entity->header;
    header->id = InvalidEntityHeaderId;
    freeEntityHeaders.Return(header);
}

static void SetOwnershipBasedOnFlag(std::unordered_set<Entity*>& set, Entity* entity, EntityFlags flag)
{
	if(entity->flags.HasFlag(flag)) set.insert(entity);
	else set.erase(entity);
}

void EntityManager::AddInterfaces(Entity* entity)
{
	SetOwnershipBasedOnFlag(updatables, entity, EntityFlags::EnableUpdate);
	SetOwnershipBasedOnFlag(fixedUpdatables, entity, EntityFlags::EnableFixedUpdate);
	SetOwnershipBasedOnFlag(serverUpdatables, entity, EntityFlags::EnableServerUpdate);
	SetOwnershipBasedOnFlag(serverFixedUpdatables, entity, EntityFlags::EnableServerFixedUpdate);
	SetOwnershipBasedOnFlag(renderables, entity, EntityFlags::EnableRender);
	SetOwnershipBasedOnFlag(hudRenderables, entity, EntityFlags::EnableRenderHud);
}

void EntityManager::ScheduleUpdateInterfaces(Entity* entity)
{
	needsUpdatedInterfaces.insert(entity);
}

void EntityManager::UpdateInterfaces()
{
	for (auto entity : needsUpdatedInterfaces)
	{
		AddInterfaces(entity);
	}

	needsUpdatedInterfaces.clear();
}
