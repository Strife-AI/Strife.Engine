#pragma once

#include <Resource/SpriteResource.hpp>
#include "Scene/EntityComponent.hpp"

struct PathFollowerComponent;

DEFINE_COMPONENT(IsometricSpriteComponent)
{
    IsometricSpriteComponent(PathFollowerComponent* pathFollower = nullptr)
        : pathFollower(pathFollower)
    {

    }

    void Render(Renderer* renderer) override;

    int layer = 0;
    SpriteResource* sprite = nullptr;
    Vector2 offsetFromCenter;
    Vector2 baseSize;
    PathFollowerComponent* pathFollower;
};