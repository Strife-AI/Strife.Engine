#include "SpriteComponent.hpp"

#include "Renderer.hpp"

SpriteComponent::SpriteComponent(StringId spriteName)
{
    SetSprite(spriteName);
}

void SpriteComponent::OnAdded()
{
    componentFlags.SetFlag(EntityComponentFlags::ReceivesRenderEvents);
}

void SpriteComponent::Render(Renderer* renderer)
{
    renderer->RenderSprite(
        sprite.Value(),
        owner->Center() + offsetFromCenter - sprite->Bounds().Size() / 2,
        depth,
        Vector2(1),
        owner->Rotation(),
        flags.HasFlag(SpriteComponentFlags::FlipHorizontal),
        blendColor);
}

void SpriteComponent::SetSprite(StringId name)
{
    sprite = ResourceManager::GetResource<Sprite>(name);
}
