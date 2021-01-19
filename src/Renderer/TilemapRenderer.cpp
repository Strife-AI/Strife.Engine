#include "TilemapRenderer.hpp"

#include "Camera.hpp"
#include "Renderer.hpp"
#include "Resource/SpriteResource.hpp"
#include "Scene/Scene.hpp"

void TilemapLayerRenderer::Render(SpriteBatcher* batcher, Camera* camera, float depth) const
{
}

void TilemapRenderer::SetMapSegment(const MapSegment* mapSegment, Scene* scene)
{
    _scene = scene;

    for (auto& layer : mapSegment->layers)
    {
        _layers.emplace_back(&layer);
    }
}

#define DEFINE_RENDER_LAYER(layer_, name_) static constexpr float name_ = (float)layer_ / 1000;


DEFINE_RENDER_LAYER(-510, DebugRenderLayer)
DEFINE_RENDER_LAYER(-505, ConsoleRenderLayer)
DEFINE_RENDER_LAYER(-25, SceneTransitionLayer)
DEFINE_RENDER_LAYER(-20, UiRenderLayer)
DEFINE_RENDER_LAYER(-18, FadeOutRenderLayer)
DEFINE_RENDER_LAYER(-17, HudRenderLayer)
DEFINE_RENDER_LAYER(-15, FadeRenderLayer)
DEFINE_RENDER_LAYER(-10, ParticlesDetailsRenderLayer)
DEFINE_RENDER_LAYER(-5, TilemapForegroundRenderLayer)
DEFINE_RENDER_LAYER(-3, FrontDetailsRenderLayer)
DEFINE_RENDER_LAYER(-1, FullbrightCharacterRenderLayer)
DEFINE_RENDER_LAYER(0, CharacterRenderLayer)
DEFINE_RENDER_LAYER(1, InteractablesRenderLayer)
DEFINE_RENDER_LAYER(2, PropsRenderLayer)
DEFINE_RENDER_LAYER(3, BackgroundDetailsRenderLayer)
DEFINE_RENDER_LAYER(10, TilemapCollisionLayer)
DEFINE_RENDER_LAYER(15, PlatformsRenderLayer)
DEFINE_RENDER_LAYER(20, TilemapBackgroundLayer)


#undef DEFINE_RENDER_LAYER

void TilemapRenderer::Render(Renderer* renderer) const
{
    int layerId = 0;

    for (auto& layer : _layers)
    {
        auto& map = layer.GetMapLayer()->tileMap;
        for (int i = 0; i < map.Rows(); ++i)
        {
            for (int j = 0; j < map.Cols(); ++j)
            {
                if (map[i][j] == nullptr) continue;

                auto position = _scene->isometricSettings.TileToWorld(Vector2(j, i));
                renderer->RenderSprite(&map[i][j]->sprite, position - Vector2(32, 32), _scene->isometricSettings.GetTileDepth(position, layerId));
            }
        }
        ++layerId;
    }
}

void TilemapRenderer::SetOffset(Vector2 offset)
{
    offset.x = round(offset.x);
    offset.y = round(offset.y);

    for(auto& layer : _layers)
    {
        layer.SetOffset(offset);
    }
}
