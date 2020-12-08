#pragma once

#include "Components/NetComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Scene/BaseEntity.hpp"
#include "Scene/IEntityEvent.hpp"

DEFINE_ENTITY(CastleEntity, "castle"), IUpdatable
{
    void OnAdded(const EntityDictionary & properties) override;
    void Update(float deltaTime) override;

    void ReceiveServerEvent(const IEntityEvent& ev) override;

private:
    float _colorChangeTime = 0;
    SpriteComponent* spriteComponent;

    SyncVar<bool> _drawRed { false };

    int _playerCount = 0;
};