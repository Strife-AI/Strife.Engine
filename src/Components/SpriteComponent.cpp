#include "SpriteComponent.hpp"

#include "Renderer.hpp"

SpriteComponent::SpriteComponent(StringId spriteName)
{
    SetSprite(spriteName);
}

void SpriteComponent::Render(Renderer* renderer)
{
    renderer->RenderSprite(
        sprite.Value(),
        owner->Center() + offsetFromCenter - sprite->Bounds().Size() * scale / 2,
        depth,
        scale,
        owner->Rotation(),
        flags.HasFlag(SpriteComponentFlags::FlipHorizontal),
        blendColor);
}

void SpriteComponent::SetSprite(StringId name)
{
    sprite = ResourceManager::GetResource<Sprite>(name);
}