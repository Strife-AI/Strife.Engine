#include "IsometricSpriteComponent.hpp"
#include "Scene/Scene.hpp"
#include "Renderer/Renderer.hpp"
#include "Components/PathFollowerComponent.hpp"

void IsometricSpriteComponent::Render(Renderer* renderer)
{
    if (pathFollower != nullptr)
    {
        layer = pathFollower->currentLayer.Value() + 1;
    }

    auto spriteBounds = sprite->sprite.Bounds().Size();
    auto position = owner->ScreenCenter() + offsetFromCenter - spriteBounds + baseSize / 2;

    auto depth = -1;//GetScene()->isometricSettings.GetTileDepth(owner->Center(), layer, layerOffset);

    renderer->RenderSprite(&sprite->sprite, position, depth);
}
