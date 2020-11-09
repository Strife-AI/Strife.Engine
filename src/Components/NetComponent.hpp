#pragma once

#include <vector>

#include "Math/Vector2.hpp"
#include "Memory/CircularQueue.hpp"
#include "Scene/EntityComponent.hpp"

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

    // Client only
    float timeRecorded;

    // Server only
    Vector2 positionAtStartOfCommand;
    PlayerCommandStatus status = PlayerCommandStatus::NotStarted;
    int fixedUpdateStartId;
};

struct PlayerSnapshot
{
    Vector2 position;
    float time;
    int commandId;
};

DEFINE_COMPONENT(NetComponent)
{
    void OnAdded() override;
    void OnRemoved() override;

    PlayerCommand* GetCommandById(int id);
    Vector2 PositionAtFixedUpdateId(int fixedUpdateId, int currentFixedUpdateId);
    Vector2 GetSnapshotPosition(float time);
    void AddSnapshot(const PlayerSnapshot & snapshot);

    int netId;
    unsigned int keyBits = 0;
    Vector2 smoothVelocity;
    unsigned int nextCommandSequenceNumber = 0;
    unsigned int lastServerSequenceNumber = 0;
    unsigned int lastServedExecuted = 0;
    Vector2 positionAtStartOfCommand;

    unsigned int clientClock = 0;

    CircularQueue<PlayerCommand, 128> commands;

    bool isClientPlayer = false;
    std::vector<PlayerSnapshot> snapshots;
};
