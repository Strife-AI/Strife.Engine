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

    float layer = 0;
    std::optional<int> layerOffset;
    SpriteResource* sprite = nullptr;
    Vector2 offsetFromCenter;
    Vector2 baseSize;
    PathFollowerComponent* pathFollower;
};