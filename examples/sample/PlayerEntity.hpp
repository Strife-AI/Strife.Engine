#pragma once

#include <queue>


#include "Memory/CircularQueue.hpp"
#include "Scene/BaseEntity.hpp"

struct PlayerEntity;

DEFINE_EVENT(PlayerAddedToGame)
{
    PlayerAddedToGame(PlayerEntity* player_)
        : player(player_)
    {
        
    }

    PlayerEntity* player;
};

DEFINE_EVENT(PlayerRemovedFromGame)
{
    PlayerRemovedFromGame(PlayerEntity* player_)
        : player(player_)
    {

    }

    PlayerEntity* player;
};

enum class PlayerCommandStatus
{
    NotStarted,
    InProgress,
    Complete
};

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

DEFINE_ENTITY(PlayerEntity, "player"), IRenderable
{
    void OnAdded(const EntityDictionary& properties) override;
    void OnDestroyed() override;

    void Render(Renderer* renderer) override;

    void SetMoveDirection(Vector2 direction);

    Vector2 PositionAtFixedUpdateId(int fixedUpdateId, int currentFixedUpdateId);
    PlayerCommand* GetCommandById(int id);

    Vector2 GetSnapshotPosition(float time);
    void AddSnapshot(const PlayerSnapshot& snapshot);

    RigidBodyComponent* rigidBody;
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