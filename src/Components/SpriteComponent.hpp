#pragma once
#include "Renderer/Color.hpp"
#include "Renderer/Sprite.hpp"
#include "Memory/ResourceManager.hpp"
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
    SpriteComponent(StringId spriteName);

    void Render(Renderer* renderer) override;

    void SetSprite(StringId name);

    Resource<Sprite> sprite;
    Vector2 offsetFromCenter;
    float depth = 0;
    Vector2 scale = Vector2::Unit();
    Flags<SpriteComponentFlags> flags;
    Color blendColor;
};
