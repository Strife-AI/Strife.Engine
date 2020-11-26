#pragma once

#include "ML.hpp"
#include "Components/NetComponent.hpp"
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

enum class PlayerState
{
    None,
    Moving,
    Attacking
};

DEFINE_ENTITY(PlayerEntity, "player"), IRenderable, IServerFixedUpdatable, IServerUpdatable, INeuralNetworkEntity<PlayerNetwork>
{
    void OnAdded(const EntityDictionary& properties) override;
    void ReceiveEvent(const IEntityEvent& ev) override;
    void ReceiveServerEvent(const IEntityEvent& ev) override;
    void OnDestroyed() override;

    void Render(Renderer* renderer) override;
    void ServerFixedUpdate(float deltaTime) override;
    void ServerUpdate(float deltaTime) override;
    void SetMoveDirection(Vector2 direction);

    void CollectData(InputType& outInput) override;
    void ReceiveDecision(OutputType& output) override;

    RigidBodyComponent* rigidBody;
    NetComponent* net;

    EntityReference<Entity> attackTarget;
    PlayerState state = PlayerState::None;
    float updateTargetTimer = 0;
    float attackCoolDown = 0;

    SyncVar<float> health{ 100, SyncVarInterpolation::None, SyncVarUpdateFrequency::Infrequent };
    SyncVar<bool> showAttack{ false, SyncVarInterpolation::None, SyncVarUpdateFrequency::Infrequent };
    SyncVar<Vector2> attackPosition { { 0, 0}, SyncVarInterpolation::Linear };
};