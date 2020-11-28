#pragma once
#include <unordered_set>
#include <vector>

#include "Entity.hpp"
#include "Memory/FreeList.hpp"

struct INeuralNetworkEntity;

constexpr int MaxEntities = 8192;

template<typename TInterface>
using EntityList = std::unordered_set<TInterface>;

struct EntityManager
{
    static constexpr int InvalidEntityHeaderId = -2;

    EntityManager();

    void RegisterEntity(Entity* entity);
    void UnregisterEntity(Entity* entity);

    template <typename TContainer, typename TItem>
    void AddIfImplementsInterface(EntityList<TContainer>& container, const TItem& item);

    template <typename TContainer, typename TItem>
    void RemoveIfImplementsInterface(EntityList<TContainer>& container, const TItem& item);

    EntityList<Entity*> entities;
    FreeList<EntityHeader> freeEntityHeaders;

    FixedSizeVector<EntityHeader, MaxEntities> entityHeaders;

    EntityList<IUpdatable*> updatables;
    EntityList<IServerUpdatable*> serverUpdatables;
    EntityList<IFixedUpdatable*> fixedUpdatables;
    EntityList<IServerFixedUpdatable*> serverFixedUpdatables;
    EntityList<IRenderable*> renderables;
    EntityList<IHudRenderable*> hudRenderables;
    EntityList<INeuralNetworkEntity*> neuralNetworkEntities;

    std::vector<Entity*> toBeDestroyed;

    int _nextEntityId = 1;
};

template <typename TContainer, typename TItem>
void EntityManager::AddIfImplementsInterface(EntityList<TContainer>& container, const TItem& item)
{
    TContainer asContainer;
    if ((asContainer = dynamic_cast<TContainer>(item)) != nullptr)
    {
        container.insert(asContainer);
    }
}

template <typename TContainer, typename TItem>
void EntityManager::RemoveIfImplementsInterface(EntityList<TContainer>& container, const TItem& item)
{
    TContainer asContainer;
    if ((asContainer = dynamic_cast<TContainer>(item)) != nullptr)
    {
        container.erase(asContainer);
    }
}
