#pragma once
#include "PlayerEntity.hpp"
#include "Scene/IEntityEvent.hpp"
#include "Scene/Scene.hpp"


struct InputService : ISceneService
{
    void OnAdded() override;

    void HandleInput();
    void Render(Renderer* renderer);
    void ReceiveEvent(const IEntityEvent& ev) override;

    PlayerEntity* GetPlayerByNetId(int netId);

    EntityReference<PlayerEntity> activePlayer;
    std::vector<PlayerEntity*> players;
};
