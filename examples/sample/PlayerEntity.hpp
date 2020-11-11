#pragma once

#include <queue>



#include "Components/NetComponent.hpp"
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

DEFINE_ENTITY(PlayerEntity, "player"), IRenderable
{
    void OnAdded(const EntityDictionary& properties) override;
    void OnEvent(const IEntityEvent& ev) override;
    void OnDestroyed() override;

    void Render(Renderer* renderer) override;

    void SetMoveDirection(Vector2 direction);

    RigidBodyComponent* rigidBody;
    NetComponent* net;
};