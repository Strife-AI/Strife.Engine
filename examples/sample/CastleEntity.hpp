#pragma once

#include "Components/SpriteComponent.hpp"
#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(CastleEntity, "castle"), IUpdatable
{
    void OnAdded(const EntityDictionary & properties) override;
    void OnEvent(const IEntityEvent& ev) override;
    void Update(float deltaTime) override;

private:
    float _colorChangeTime = 0;
    SpriteComponent* spriteComponent;
};