#pragma once

#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(PlayerEntity, "player"), IRenderable
{
    void OnAdded(const EntityDictionary& properties) override;
    void Render(Renderer* renderer) override;
};