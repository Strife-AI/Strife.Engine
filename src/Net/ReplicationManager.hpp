#pragma once
#include <unordered_map>

#include "Math/Vector2.hpp"
#include "Scene/Entity.hpp"

struct EntityState
{
    int netId;
    StringId type;
    Vector2 position;
    EntityReference<Entity> entity;
};

struct WorldDiff
{
    std::vector<int> createdEntities;
    std::vector<int> destroyedEntities;
};

struct WorldState
{
    std::unordered_map<int, EntityState> entitiesByNetId;
};

class ReplicationManager
{
public:
    static void ComputeWorldDiff(WorldState& before, WorldState& after, WorldDiff& outDiff);

    static WorldState GetWorldState(Scene* scene);

private:
};