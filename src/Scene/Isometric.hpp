#pragma once

#include "Memory/Flags.hpp"
#include "Container/Grid.hpp"

class MapSegment;

enum class RampType
{
    None,
    West,
    North
};

enum class TerrainCellFlags
{
    Unwalkable = 1
};

struct IsometricTerrainCell
{
    int height = 0;
    RampType rampType = RampType::None;
    Flags<TerrainCellFlags> flags;
};

class PathFinderService;

struct IsometricSettings
{
    Vector2 TileToWorld(Vector2 tile) const
    {
        return Vector2(
            (tile.x - tile.y) * tileSize.x / 2,
            (tile.x + tile.y) * tileSize.y / 2);
    }

    Vector2 WorldToTile(Vector2 world) const
    {
        return Vector2(
        (2 * world.y + world.x),
        (2 * world.y - world.x) / 2) / tileSize;
    }

    Vector2 WorldToIntegerTile(Vector2 world) const
    {
        return WorldToTile(world)
            .Floor()
            .AsVectorOfType<float>()
            .Clamp({ 0, 0}, terrain.Dimensions());
    }

    void GetTileBoundaries(Vector2 tile, gsl::span<Vector2> boundaries) const
    {
        boundaries[0] = TileToWorld(tile);
        boundaries[1] = TileToWorld(tile + Vector2(1, 0));
        boundaries[2] = TileToWorld(tile + Vector2(1, 1));
        boundaries[3] = TileToWorld(tile + Vector2(0, 1));
    }

    float GetTileDepth(Vector2 position, int layer) const;
    void BuildFromMapSegment(const MapSegment& mapSegment, PathFinderService* pathFinder, struct TilemapEntity* tilemap);
    int GetCurrentLayer(Vector2 position) const;

    Vector2 tileSize;
    float baseDepth = 0;
    VariableSizedGrid<IsometricTerrainCell> terrain;
};