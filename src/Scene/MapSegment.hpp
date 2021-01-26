#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>



#include "Entity.hpp"
#include "Math/Rectangle.hpp"
#include "Math/Polygon.hpp"
#include "Renderer/Sprite.hpp"
#include "Memory/StringId.hpp"
#include "Container/Grid.hpp"

struct TileProperties
{
    TileProperties(const Sprite& sprite_, int id_, const std::map<std::string, std::string>& properties)
        : sprite(sprite_),
        id(id_),
        properties(properties)
    {

    }

    Sprite sprite;
    int id;
    std::map<std::string, std::string> properties;
};

struct MapLayer
{
    explicit MapLayer(TileProperties** tileMap, int rows, int cols, Vector2i tileSize, StringId layerName_)
        : tileMap(rows, cols, tileMap),
          tileSize(tileSize),
          layerName(layerName_)
    {

    }

    StringId layerName;
    Grid<TileProperties*> tileMap;
    Vector2i tileSize;
};

struct EntityInstance
{
    int totalProperties;
};

struct MapSegment
{
    StringId name;

    std::vector<Rectanglei> colliders;
    std::vector<Polygoni> polygonColliders;

    std::vector<MapLayer> layers;
    std::vector<EntityInstance> entities;
    std::vector<TileProperties*> tileProperties;


    // TODO: deprecate
    Vector2 startMarker;
    Vector2 endMarker;

private:
};

