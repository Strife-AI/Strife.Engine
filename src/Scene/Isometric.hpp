#pragma once

#include "Memory/Flags.hpp"
#include "Container/Grid.hpp"

class MapSegment;

enum class IsometricTerrainCellFlags
{
    Solid = 1,
    RampNorth = 2,
    RampSouth = 4,
    RampEast = 8,
    RampWest = 16
};

struct IsometricTerrainCell
{
    int height = 0;
    Flags<IsometricTerrainCellFlags> flags;
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
        Vector2 c = world / tileSize * 2;
        return Vector2(
            (c.x + c.y) / 2,
            (c.x + c.y) / 2 - c.x);
    }

    Vector2 WorldToIntegerTile(Vector2 world) const
    {
        return WorldToTile(world)
            .Floor()
            .AsVectorOfType<float>()
            .Clamp({ 0, 0}, terrain.Dimensions());
    }

    float GetTileDepth(Vector2 position, int layer) const;
    void BuildFromMapSegment(const MapSegment& mapSegment, PathFinderService* pathFinder);
    int GetCurrentLayer(Vector2 position) const;

    Vector2 tileSize;
    float baseDepth = 0;
    VariableSizedGrid<IsometricTerrainCell> terrain;
};