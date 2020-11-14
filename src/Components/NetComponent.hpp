#pragma once

#include <vector>

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

struct PlayerCommand
{
    unsigned char fixedUpdateCount;
    unsigned char keys;
    unsigned int id;

    uint32 netId;
    bool moveToTarget = false;
    Vector2 target;

    // Client only
    float timeRecorded;

    // Server only
    PlayerCommandStatus status = PlayerCommandStatus::NotStarted;
    int fixedUpdateStartId;
};

struct PlayerSnapshot
{
    Vector2 position;
    float time;
    int commandId;
};

DEFINE_EVENT(SpawnedOnClientEvent)
{

};

struct FlowField;

DEFINE_COMPONENT(NetComponent)
{
    void OnAdded() override;
    void OnRemoved() override;

    Vector2 GetSnapshotPosition(float time);
    void AddSnapshot(const PlayerSnapshot& snapshot);

    int netId;
    int ownerClientId;

    std::vector<PlayerSnapshot> snapshots;

    std::shared_ptr<FlowField> flowField;
    Vector2 acceleration;
};
