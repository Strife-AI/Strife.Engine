#pragma once

#include <Resource/SpriteResource.hpp>
#include "Scene/EntityComponent.hpp"

DEFINE_COMPONENT(IsometricSpriteComponent)
{
    void Render(Renderer* renderer) override;

    int layer = 0;
    SpriteResource* sprite = nullptr;
    Vector2 offsetFromCenter;
    Vector2 baseSize;
};