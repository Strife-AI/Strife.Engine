#pragma once

#include "Components/SpriteComponent.hpp"
#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(CastleEntity, "castle")
{
    void OnAdded(const EntityDictionary & properties) override;
    void ReceiveServerEvent(const IEntityEvent& ev) override;

    void DoNetSerialize(NetSerializer& serializer) override;

private:
    float _colorChangeTime = 0;
    SpriteComponent* spriteComponent;

    bool _drawRed = false;
    int _playerCount = 0;
};