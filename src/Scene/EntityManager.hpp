#pragma once
#include <unordered_set>
#include <vector>

#include "Entity.hpp"
#include "Memory/FreeList.hpp"

constexpr int MaxEntities = 8192;

struct EntityManager
{
    static constexpr int InvalidEntityHeaderId = -2;

    EntityManager();

    void RegisterEntity(Entity* entity);
    void UnregisterEntity(Entity* entity);

    void AddInterfaces(Entity* entity);
    void ScheduleUpdateInterfaces(Entity* entity);
    void UpdateInterfaces();

    FreeList<EntityHeader> freeEntityHeaders;

    FixedSizeVector<EntityHeader, MaxEntities> entityHeaders;

	std::unordered_set<Entity*> entities;
    std::unordered_set<Entity*> updatables;
	std::unordered_set<Entity*> serverUpdatables;
	std::unordered_set<Entity*> fixedUpdatables;
	std::unordered_set<Entity*> serverFixedUpdatables;
	std::unordered_set<Entity*> renderables;
	std::unordered_set<Entity*> hudRenderables;

    std::vector<Entity*> toBeDestroyed;
    std::unordered_set<Entity*> needsUpdatedInterfaces;

    int _nextEntityId = 1;
};