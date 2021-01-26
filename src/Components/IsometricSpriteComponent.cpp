#include "IsometricSpriteComponent.hpp"
#include "Scene/Scene.hpp"
#include "Renderer/Renderer.hpp"

void IsometricSpriteComponent::Render(Renderer* renderer)
{
    auto spriteBounds = sprite->sprite.Bounds().Size();
    auto position = owner->Center() + offsetFromCenter - spriteBounds + baseSize / 2;

    auto spriteBottomRight = position + sprite->sprite.Bounds().Size();
    auto depth = GetScene()->isometricSettings.GetTileDepth(spriteBottomRight, layer);

    renderer->RenderSprite(&sprite->sprite, position, depth);
}
