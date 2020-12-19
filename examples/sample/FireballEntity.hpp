#pragma once

#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(FireballEntity, "fireball"), IUpdatable
{
    static constexpr float Radius = 5;

    void OnAdded(const EntityDictionary& properties) override;
    void OnDestroyed() override;
    void ReceiveServerEvent(const IEntityEvent& ev) override;
    void Update(float deltaTime) override;

    PointLight light;
    int ownerId;
};