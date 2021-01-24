#include <Physics/PathFinding.hpp>
#include "Isometric.hpp"
#include "Scene/MapSegment.hpp"
#include "Scene/TilemapEntity.hpp"
#include "Components/RigidBodyComponent.hpp"

static bool TileIsRamp(TileProperties* properties)
{
    for (auto& property : properties->properties)
    {
        if (property.first == "ramp") return true;
    }

    return false;
}

void IsometricSettings::BuildFromMapSegment(const MapSegment& mapSegment, PathFinderService* pathFinder, TilemapEntity* tilemap)
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
                    if (!TileIsRamp(tile))
                    {
                        if (i > 0 && j > 0) terrain[i - 1][j - 1].height = layerId;
                    }
                    else
                    {
                        if (i > 0 && j > 0) terrain[i - 1][j - 1].height = layerId;
                        terrain[i][j].height = layerId;
                    }
                }
            }
        }
    }

    const Vector2 offsets[4] =
        {
            { 0, -1 },
            { 1, 0 },
            { 0, 1 },
            { -1, 0 },
        };

    // Add blocked edges where there's a change in elevation
    {
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

    // Ramps
    {
        for (int layerId = 1; layerId < mapSegment.layers.size(); ++layerId)
        {
            auto& layer = mapSegment.layers[layerId].tileMap;
            for (int i = 1; i < terrain.Rows() - 1; ++i)
            {
                for (int j = 1; j < terrain.Cols() - 1; ++j)
                {
                    auto tile = layer[i][j];
                    if (tile != nullptr)
                    {
                        for (auto& property : tile->properties)
                        {
                            if (property.first == "ramp")
                            {
                                if (property.second == "west") terrain[i][j].rampType = RampType::West;
                                else if (property.second == "north") terrain[i][j].rampType = terrain[i][j].rampType = RampType::North;
                            }
                        }
                    }
                }
            }
        }
    }

    // Mark tiles under elevated tiles as impassible
    {
        for (int i = 0; i < terrain.Rows(); ++i)
        {
            for (int j = 0; j < terrain.Cols(); ++j)
            {
                int height = terrain[i][j].height;

                if (terrain[i][j].rampType == RampType::None && height > 0)
                {
                    int x = j + 1;
                    int y = i + 1;

                    if (terrain[y][x].height != terrain[i][j].height && x < terrain.Cols() && y < terrain.Rows())
                    {
                        pathFinder->AddObstacle(Rectangle(x, y, 1, 1));
                        terrain[y][x].flags.SetFlag(TerrainCellFlags::Unwalkable);
                    }
                }
            }
        }
    }

    auto scene = pathFinder->scene;
    auto tilemapRb = tilemap->GetComponent<RigidBodyComponent>();

    for (int i = 1; i < terrain.Rows() - 1; ++i)
    {
        for (int j = 1; j < terrain.Cols() - 1; ++j)
        {
            Vector2 points[4] =
            {
                TileToWorld(Vector2(j, i)),
                TileToWorld(Vector2(j + 1, i)),
                TileToWorld(Vector2(j + 1, i + 1)),
            };

            for (int k = 0; k < 2; ++k)
            {
                auto to = Vector2(j, i) + offsets[k];
                if (terrain[i][j].height != terrain[to].height || terrain[to].flags.HasFlag(TerrainCellFlags::Unwalkable) || terrain[i][j].flags.HasFlag(TerrainCellFlags::Unwalkable))
                {
                    auto start = points[k];
                    auto end = points[(k + 1) % 4];
                    tilemapRb->CreateLineCollider(start, end);
                }
            }
        }
    }
}

float IsometricSettings::GetTileDepth(Vector2 position, int layer) const
{
    position = WorldToTile(position);
    float maxWidth = terrain.Cols();
    float maxHeight = terrain.Rows();

    float dz = 0.1f / (maxWidth * maxHeight * 10);

    return baseDepth - (position.x + position.y * maxWidth + layer * maxWidth * maxHeight) * dz;
}

int IsometricSettings::GetCurrentLayer(Vector2 position) const
{
    auto tilePosition = WorldToIntegerTile(position);
    return terrain[tilePosition].height;
}