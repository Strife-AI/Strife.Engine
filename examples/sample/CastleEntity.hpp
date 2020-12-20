#pragma once

#include "Components/NetComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Scene/BaseEntity.hpp"
#include "Scene/IEntityEvent.hpp"

DEFINE_ENTITY(CastleEntity, "castle")
{
    void OnAdded(const EntityDictionary & properties) override;
    void OnDestroyed() override;
    void Update(float deltaTime) override;
    void SpawnPlayer();

    void ReceiveServerEvent(const IEntityEvent& ev) override;
    void ReceiveEvent(const IEntityEvent& ev) override;

    NetComponent* net;

private:
    float _colorChangeTime = 0;
    SpriteComponent* spriteComponent;

    Vector2 _spawnSlots[4];
    int _nextSpawnSlotId = 0;

    PointLight _light;
};