#include "Isometric.hpp"
#include "Scene/MapSegment.hpp"

void IsometricSettings::BuildFromMapSegment(const MapSegment& mapSegment)
{
    // Calculate terrain size
    {
        Vector2 mapSize;
        for (auto& layer : mapSegment.layers) mapSize = mapSize.Max(layer.tileMap.Dimensions());
        terrain.SetSize(mapSize.y, mapSize.x);
    }

    // Exclude layer 0, since that has background tiles
    for (int layerId = 1; layerId < mapSegment.layers.size(); ++layerId)
    {
        auto& layer = mapSegment.layers[layerId].tileMap;
        for (int i = 0; i < layer.Rows(); ++i)
        {
            for (int j = 0; j < layer.Cols(); ++j)
            {
                if (layer[i][j] != nullptr)
                {
                    if (i > 0 && j > 0) terrain[i - 1][j - 1].height = layerId;
                }

                terrain[i][j].flags.SetFlag(IsometricTerrainCellFlags::Solid);
            }
        }
    }
}

float IsometricSettings::GetTileDepth(Vector2 position, int layer) const
{
    position = position / tileSize;
    float dz = 1e-7;
    float maxWidth = 1000;
    float maxHeight = 1000;

    return baseDepth - (position.x + position.y * maxWidth + layer * maxWidth * maxHeight) * dz;
}

int IsometricSettings::GetCurrentLayer(Vector2 position) const
{
    auto tilePosition = WorldToIntegerTile(position);
    return terrain[tilePosition].height;
}
