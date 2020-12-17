#pragma once

#include "GameML.hpp"
#include "Components/NetComponent.hpp"
#include "ML/ML.hpp"
#include "Scene/BaseEntity.hpp"
#include "Scene/IEntityEvent.hpp"
#include "HealthBarComponent.hpp"

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

enum class PlayerState
{
    None,
    Moving,
    Attacking
};

DEFINE_ENTITY(PlayerEntity, "player"), IRenderable, IServerFixedUpdatable, IServerUpdatable
{
    using NeuralNetwork = NeuralNetworkComponent<PlayerNetwork>;

    void OnAdded(const EntityDictionary& properties) override;
    void ReceiveEvent(const IEntityEvent& ev) override;
    void ReceiveServerEvent(const IEntityEvent& ev) override;
    void OnDestroyed() override;

    void Render(Renderer* renderer) override;
    void ServerFixedUpdate(float deltaTime) override;
    void ServerUpdate(float deltaTime) override;
    void SetMoveDirection(Vector2 direction);

    RigidBodyComponent* rigidBody;
    NetComponent* net;
    HealthBarComponent* health;

    EntityReference<Entity> attackTarget;
    PlayerState state = PlayerState::None;
    float updateTargetTimer = 0;
    float attackCoolDown = 0;

    SyncVar<bool> showAttack{ false, SyncVarInterpolation::None, SyncVarUpdateFrequency::Infrequent };
    SyncVar<Vector2> attackPosition { { 0, 0}, SyncVarInterpolation::Linear };
};