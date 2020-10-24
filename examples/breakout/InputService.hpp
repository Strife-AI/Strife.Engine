#pragma once
#include "PlayerEntity.hpp"
#include "Scene/IEntityEvent.hpp"
#include "Scene/Scene.hpp"


struct InputService : ISceneService
{
    void HandleInput();
    void Render(Renderer* renderer);
    void ReceiveEvent(const IEntityEvent& ev) override;
    void SwitchControlledPlayer(PlayerEntity* player);
    PlayerEntity* FindClosestPlayerOrNull(Vector2 point);

    EntityReference<PlayerEntity> activePlayer;
    std::vector<PlayerEntity*> players;
};
