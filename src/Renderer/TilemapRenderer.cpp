#include "TilemapRenderer.hpp"

#include "Camera.hpp"
#include "Renderer.hpp"

void TilemapLayerRenderer::Render(SpriteBatcher* batcher, Camera* camera, float depth) const
{
    auto& tileMap = _layer->tileMap;
    auto tileSize = _layer->tileSize.AsVectorOfType<float>();

    Rectangle bounds(_offset, Vector2(tileMap.Cols(), tileMap.Cols()) * tileSize);

    if (!camera->Bounds().IntersectsWith(bounds))
    {
        return;
    }

    auto cameraBounds = camera->Bounds();
    auto topLeft = ((cameraBounds.TopLeft() - bounds.TopLeft()) / tileSize)
        .Floor()
        .Max(Vector2i(0, 0));
    auto bottomRight = ((cameraBounds.BottomRight() - bounds.TopLeft()) / tileSize)
        .Floor()
        .Min(Vector2i(tileMap.Cols() - 1, tileMap.Rows() - 1));

    for (int i = topLeft.y; i <= bottomRight.y; ++i)
    {
        for (int j = topLeft.x; j <= bottomRight.x; ++j)
        {
            TileProperties* tile = tileMap[i][j];
            if (tile != nullptr)
            {
                batcher->RenderSprite(
                    tile->sprite,
                    Vector3(_offset + Vector2(j, i) * tileSize, depth),
                    Vector2(1, 1),
                    0,
                    false,
                    Color());
            }
        }
    }

    batcher->Render();
}

void TilemapRenderer::SetMapSegment(const MapSegment* mapSegment)
{
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
    auto depth = TilemapBackgroundLayer;

    for (auto& layer : _layers)
    {
        float z;

        //if(layer.GetMapLayer()->layerName == "Foreground"_sid)
        //{
        //    z = TilemapForegroundRenderLayer;
        //}
        //else if(layer.GetMapLayer()->layerName == "Collision"_sid)
        //{
        //    z = TilemapCollisionLayer;;
        //}
        //else
        {
            z = depth;
            depth -= 0.00001;
        }

        renderer->RenderCustomTransparency(&layer, z);
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
