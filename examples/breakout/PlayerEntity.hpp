#pragma once

#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(PlayerEntity, "player"), IRenderable, IUpdatable
{
    void OnAdded(const EntityDictionary& properties) override;
    void Render(Renderer* renderer) override;
    void Update(float deltaTime) override;

    RigidBodyComponent* rigidBody;
};