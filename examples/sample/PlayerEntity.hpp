#pragma once

#include <queue>

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

struct PlayerCommand
{
    unsigned char timeMilliseconds;
    unsigned char keys;
    unsigned int id;
    float sentTime = -1;
    float receivedResponseTime = -1;
};

DEFINE_ENTITY(PlayerEntity, "player"), IRenderable
{
    void OnAdded(const EntityDictionary& properties) override;
    void OnDestroyed() override;

    void Render(Renderer* renderer) override;

    void SetMoveDirection(Vector2 direction);

    RigidBodyComponent* rigidBody;
    int netId;
    unsigned int keyBits = 0;
    Vector2 smoothVelocity;
    unsigned int nextCommandSequenceNumber = 0;
    unsigned int lastServerSequenceNumber = 0;
    unsigned int lastServedExecuted = 0;

    unsigned int clientClock = 0;

    std::list<PlayerCommand> commands;
};