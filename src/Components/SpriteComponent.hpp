#pragma once

#include <Resource/SpriteResource.hpp>
#include "Renderer/Color.hpp"
#include "Renderer/Sprite.hpp"
#include "Scene/EntityComponent.hpp"

enum class SpriteComponentFlags
{
    FlipHorizontal = 1,
    FlipVertical = 2,   // TODO
    RenderFullbright = 4
};

DEFINE_COMPONENT(SpriteComponent)
{
    SpriteComponent() = default;
    SpriteComponent(const char* spriteName);

    void Render(Renderer* renderer) override;

    void SetSprite(const char* name);

    SpriteResource* sprite;
    Vector2 offsetFromCenter;
    float depth = 0;
    Vector2 scale = Vector2::Unit();
    Flags<SpriteComponentFlags> flags;
    Color blendColor;
};
