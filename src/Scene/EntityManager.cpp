#include "EntityManager.hpp"
#include "ML/ML.hpp"

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

    AddIfImplementsInterface(updatables, entity);
    AddIfImplementsInterface(serverUpdatables, entity);

    AddIfImplementsInterface(fixedUpdatables, entity);
    AddIfImplementsInterface(serverFixedUpdatables, entity);

    AddIfImplementsInterface(renderables, entity);
    AddIfImplementsInterface(hudRenderables, entity);

    EntityHeader* header = freeEntityHeaders.Borrow();

    header->id = _nextEntityId++;
    header->entity = entity;

    entity->id = header->id;
    entity->header = header;
}

void EntityManager::UnregisterEntity(Entity* entity)
{
    entities.erase(entity);

    RemoveIfImplementsInterface(updatables, entity);
    RemoveIfImplementsInterface(serverUpdatables, entity);

    RemoveIfImplementsInterface(fixedUpdatables, entity);
    RemoveIfImplementsInterface(serverFixedUpdatables, entity);

    RemoveIfImplementsInterface(renderables, entity);
    RemoveIfImplementsInterface(hudRenderables, entity);

    EntityHeader* header = entity->header;
    header->id = InvalidEntityHeaderId;
    freeEntityHeaders.Return(header);
}
