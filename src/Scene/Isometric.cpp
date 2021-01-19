#include <Physics/PathFinding.hpp>
#include "Isometric.hpp"
#include "Scene/MapSegment.hpp"

void IsometricSettings::BuildFromMapSegment(const MapSegment& mapSegment, PathFinderService* pathFinder)
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
                auto tile = layer[i][j];
                if (tile != nullptr)
                {
                    if (i > 0 && j > 0) terrain[i - 1][j - 1].height = layerId;

                    for (auto& property : tile->properties)
                    {
                        if (property.first == "ramp")
                        {
                            Log("Found ramp\n");
                        }
                    }

                    terrain[i][j].flags.SetFlag(IsometricTerrainCellFlags::Solid);
                }
            }
        }
    }

    // Add blocked edges where there's a change in elevation
    {
        const Vector2 offsets[4] =
        {
            { 0, 1 },
            { 0, -1 },
            { -1, 0 },
            { 1, 0 },
        };

        for (int i = 1; i < terrain.Rows() - 1; ++i)
        {
            for (int j = 1; j < terrain.Cols() - 1; ++j)
            {
                Vector2 from = Vector2(j, i);

                for (auto offset : offsets)
                {
                    Vector2 to = from + offset;
                    if (terrain[from].height != terrain[to].height)
                    {
                        pathFinder->AddEdge(from, to);
                    }
                }
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
