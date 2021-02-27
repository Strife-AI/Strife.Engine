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
    IsometricSettings(Scene* scene)
        : scene(scene)
    {

    }

    Vector2 TileToScreen(Vector2 tile) const
    {
        return TileToScreen(tile, tileSize);
    }

    Vector2 TileToWorld(Vector2 tile) const
    {
        return tile * Vector2(32);
    }

    Vector2 TileToScreenIncludingTerrain(Vector2 tile);

    static Vector2 TileToScreen(Vector2 tile, Vector2 tileSize)
    {
        return Vector2(
            (tile.x - tile.y) * tileSize.x / 2,
            (tile.x + tile.y) * tileSize.y / 2);
    }

    Vector2 ScreenToTile(Vector2 screen) const
    {
        return ScreenToTile(screen, tileSize);
    }

    Vector2 ScreenToWorld(Vector2 screen) const
    {
        return TileToWorld(ScreenToTile(screen));
    }

    Vector2 ScreenToWorldIncludingTerrain(Vector2 screen) const
    {
        auto tile = ScreenToTile(screen);
        return TileToWorld(tile + Vector2(terrain[tile].height));
    }

    Vector2 WorldToScreen(Vector2 world)
    {
        return TileToScreen(world / 32);
    }

    Vector2 WorldToTile(Vector2 world)
    {
        return world / 32;
    }

    static Vector2 ScreenToTile(Vector2 world, Vector2 tileSize)
    {
        return Vector2(
            (2 * world.y + world.x),
            (2 * world.y - world.x) / 2) / tileSize;
    }

    Vector2 ScreenToIntegerTile(Vector2 world) const
    {
        return ScreenToTile(world)
            .Floor()
            .AsVectorOfType<float>()
            .Clamp({ 0, 0}, terrain.Dimensions());
    }

    Vector2 ScreenToIntegerTile(Vector2 world, Vector2 tileSize)
    {
        return ScreenToTile(world, tileSize)
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

    float GetTileDepth(Vector2 position, int layer, std::optional<int> layerOffset = std::nullopt) const;
    void BuildFromMapSegment(const MapSegment& mapSegment, PathFinderService* pathFinder, struct TilemapEntity* tilemap);
    int GetCurrentLayer(Vector2 position) const;

    Vector2 tileSize;
    float baseDepth = 0;
    VariableSizedGrid<IsometricTerrainCell> terrain;
    Scene* scene;
};