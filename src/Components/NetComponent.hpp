#pragma once

#include <vector>
#include <slikenet/BitStream.h>
#include <memory>

#include "Math/Vector2.hpp"
#include "Memory/CircularQueue.hpp"
#include "Scene/EntityComponent.hpp"
#include "Scene/IEntityEvent.hpp"

enum class PlayerCommandStatus
{
    NotStarted,
    InProgress,
    Complete
};

Vector2 GetDirectionFromKeyBits(unsigned int keyBits);

DEFINE_EVENT(MoveToEvent)
{
    Vector2 position;
};

DEFINE_EVENT(AttackEvent)
{
    Entity* entity;
};

struct PlayerCommand
{
    unsigned char fixedUpdateCount;
    unsigned char keys;
    unsigned int id;

    uint32 netId;
    bool moveToTarget = false;
    Vector2 target;

    bool attackTarget = false;
    uint32 attackNetId;

    // Client only
    float timeRecorded;

    // Server only
    PlayerCommandStatus status = PlayerCommandStatus::NotStarted;
    int fixedUpdateStartId;
};

DEFINE_EVENT(SpawnedOnClientEvent)
{

};

struct FlowField;

DEFINE_COMPONENT(NetComponent)
{
    void OnAdded() override;
    void OnRemoved() override;

    int netId = -2;
    int ownerClientId = -2;
    bool isMarkedForDestructionOnClient = false;

    float destroyTime = INFINITY;
    bool markedForDestruction = false;
};
