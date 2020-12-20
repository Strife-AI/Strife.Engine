#pragma once

#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(PuckEntity, "puck")
{
    void OnAdded(const EntityDictionary& properties) override;
    void Render(Renderer* renderer) override;

    void ChangeDirection();
};