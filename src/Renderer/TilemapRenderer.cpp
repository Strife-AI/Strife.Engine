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

void TilemapRenderer::Render(Renderer* renderer) const
{
    // FIXME: MW
    //auto depth = TilemapBackgroundLayer;

    //for (auto& layer : _layers)
    //{
    //    float z;

    //    if(layer.GetMapLayer()->layerName == "Foreground"_sid)
    //    {
    //        z = TilemapForegroundRenderLayer;
    //    }
    //    else if(layer.GetMapLayer()->layerName == "Collision"_sid)
    //    {
    //        z = TilemapCollisionLayer;;
    //    }
    //    else
    //    {
    //        z = depth;
    //        depth -= 0.00001;
    //    }

    //    renderer->RenderCustomTransparency(&layer, z);
    //}
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
