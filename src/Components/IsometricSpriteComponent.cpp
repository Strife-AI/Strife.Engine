#include "IsometricSpriteComponent.hpp"
#include "Scene/Scene.hpp"
#include "Renderer/Renderer.hpp"
#include "Components/PathFollowerComponent.hpp"

void IsometricSpriteComponent::Render(Renderer* renderer)
{
    auto spriteBounds = sprite->sprite.Bounds().Size();
    auto position = owner->ScreenCenter() + offsetFromCenter - spriteBounds + baseSize / 2;
    int layer = GetScene()->GetService<PathFinderService>()->GetCell(GetScene()->isometricSettings.WorldToTile(owner->Center())).height;

    auto depth = GetScene()->isometricSettings.GetTileDepth(owner->Center(), layer + 1, layerOffset);

    renderer->RenderSprite(&sprite->sprite, position, depth);
}
