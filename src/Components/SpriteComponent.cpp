#include "SpriteComponent.hpp"

#include "Renderer.hpp"

SpriteComponent::SpriteComponent(const char* spriteName)
{
    SetSprite(spriteName);
}

void SpriteComponent::Render(Renderer* renderer)
{
    renderer->RenderSprite(
        &sprite->sprite,
        owner->Center() + offsetFromCenter - sprite->sprite.Bounds().Size() * scale / 2,
        depth,
        scale,
        owner->Rotation(),
        flags.HasFlag(SpriteComponentFlags::FlipHorizontal),
        blendColor);
}

void SpriteComponent::SetSprite(const char* name)
{
    sprite = GetResource<SpriteResource>(name);
}