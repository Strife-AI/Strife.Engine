#pragma once

#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(PuckEntity, "puck"), IRenderable
{
    void OnAdded(const EntityDictionary& properties) override;
    void Render(Renderer* renderer) override;
};