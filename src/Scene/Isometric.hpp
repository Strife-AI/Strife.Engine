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

struct IsometricTerrainCell
{
    int height = 0;
    RampType rampType = RampType::None;
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

    float GetTileDepth(Vector2 position, int layer) const;
    void BuildFromMapSegment(const MapSegment& mapSegment, PathFinderService* pathFinder);
    int GetCurrentLayer(Vector2 position) const;

    Vector2 tileSize;
    float baseDepth = 0;
    VariableSizedGrid<IsometricTerrainCell> terrain;
};