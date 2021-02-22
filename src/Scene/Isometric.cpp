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
                int height = layerId;
                if (tile != nullptr)
                {
                    if (!TileIsRamp(tile))
                    {
                        if (i > 0 && j > 0)
                        {
                            terrain[i - 1][j - 1].height = height;

                            if (j + height < terrain.Cols() && i + height < terrain.Rows())
                            {
                                pathFinder->GetCell(Vector2(j, i) + Vector2(height - 1)).height = height;
                            }
                        }
                    }
                    else
                    {
                        if (i > 0 && j > 0) terrain[i - 1][j - 1].height = layerId;

                        if (j + height < terrain.Cols() && i + height < terrain.Rows())
                        {
                            auto& cell = pathFinder->GetCell(Vector2(j + height - 1, i + height - 1));
                            cell.height = 0;

                            for (auto& property : tile->properties)
                            {
                                if (property.first == "ramp")
                                {
                                    if (property.second == "west") cell.ramp = ObstacleRampType::West;
                                    else if (property.second == "north") cell.ramp = ObstacleRampType::North;
                                }
                            }
                        }
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
                auto& fromCell = pathFinder->GetCell(from);

                for (auto offset : offsets)
                {
                    Vector2 to = from + offset;

                    auto& toCell = pathFinder->GetCell(to);
                    if (toCell.ramp != ObstacleRampType::None) continue;

                    switch (fromCell.ramp)
                    {
                        case ObstacleRampType::None:
                        {
                            if (fromCell.height != pathFinder->GetCell(to).height)
                            {
                                pathFinder->AddEdge(from, to);
                            }
                            break;
                        }

                        case ObstacleRampType::West:
                        {
                            if (offset == Vector2(-1, 0) || offset == Vector2(1, 0))
                            {
                                continue;
                            }

                            pathFinder->AddEdge(from, to);

                            break;
                        }
                    };
                }
            }
        }
    }

#if false
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


    // Add line collider between walkable/unwalkable ares
    {
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
                    if (terrain[i][j].height != terrain[to].height
                        || terrain[to].flags.HasFlag(TerrainCellFlags::Unwalkable)
                        || terrain[i][j].flags.HasFlag(TerrainCellFlags::Unwalkable)
                        || terrain[i + 1][j + 1].rampType != RampType::None
                        || terrain[(int)to.y + 1][(int)to.x + 1].rampType != RampType::None)
                    {
                        auto start = points[k];
                        auto end = points[(k + 1) % 4];

                        bool addAsTrigger = terrain[i][j].rampType != RampType::None
                                            || terrain[to].rampType != RampType::None
                                            || terrain[i + 1][j + 1].rampType != RampType::None
                                            || terrain[to + Vector2(1)].rampType != RampType::None;

                        tilemapRb->CreateLineCollider(start, end, addAsTrigger);
                    }
                }
            }
        }
    }

#endif

    auto scene = pathFinder->scene;

    // Create walkable ares for the grid sensor
    {
        auto walkable0 = scene->CreateEntity<WalkableTerrainEntity0>({});
        auto walkable1 = scene->CreateEntity<WalkableTerrainEntity1>({});
        auto walkable2 = scene->CreateEntity<WalkableTerrainEntity2>({});
        auto rampEntity = scene->CreateEntity<RampEntity>({});

        for (int i = 0; i < terrain.Rows(); ++i)
        {
            for (int j = 0; j < terrain.Cols(); ++j)
            {
                Vector2 points[4];
                GetTileBoundaries(Vector2(j, i), points);

                if (terrain[i][j].rampType != RampType::None)
                {
                    rampEntity->rigidBody->CreatePolygonCollider(points, true);
                }

                if (terrain[i][j].height == 0) walkable0->rigidBody->CreatePolygonCollider(points, true);
                if (terrain[i][j].height == 1) walkable1->rigidBody->CreatePolygonCollider(points, true);
                if (terrain[i][j].height == 2) walkable2->rigidBody->CreatePolygonCollider(points, true);
            }
        }
    }
}

float IsometricSettings::GetTileDepth(Vector2 position, int layer, std::optional<int> layerOffset) const
{
    position = WorldToTile(position) + (layerOffset.has_value() ? Vector2(layerOffset.value()) : Vector2(layer));
    float maxWidth = terrain.Cols();
    float maxHeight = terrain.Rows();

    float maxLayers = 10;
    float dz = 0.1f / (maxWidth * maxHeight * maxLayers);

    return baseDepth - (position.x + position.y) * dz * maxLayers - layer * dz;
}

int IsometricSettings::GetCurrentLayer(Vector2 position) const
{
    auto tilePosition = WorldToIntegerTile(position);
    return terrain[tilePosition].height;
}