#include "SpriteComponent.hpp"
#include "Renderer.hpp"
#include "Scene/Scene.hpp"

SpriteComponent::SpriteComponent(const char* spriteName)
{
    SetSprite(spriteName);
}

void SpriteComponent::Render(Renderer* renderer)
{
    auto scene = GetScene();
    float spriteDepth = scene->perspective == ScenePerspective::Orothgraphic
        ? depth
        : scene->isometricSettings.GetTileDepth(owner->Center(), depth);

    renderer->RenderSprite(
        &sprite->Get(),
        owner->Center() + offsetFromCenter - sprite->Get().Bounds().Size() * scale / 2,
        spriteDepth,
        scale,
        owner->Rotation(),
        flags.HasFlag(SpriteComponentFlags::FlipHorizontal),
        blendColor);
}

void SpriteComponent::SetSprite(const char* name)
{
    sprite = GetResource<SpriteResource>(name);
}