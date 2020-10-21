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
#include "Memory/Grid.hpp"

struct TileProperties
{
    TileProperties(const Sprite& sprite_, int id_)
        : sprite(sprite_),
    id(id_)
    {

    }

    Sprite sprite;
    int id;
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
    std::unique_ptr<EntityProperty[]> properties;
    int totalProperties;
};

struct MapSegment
{
    void SetProperties(const std::map<std::string, std::string>& mapProperties);

    StringId name;

    std::vector<Rectanglei> colliders;
    std::vector<Polygoni> polygonColliders;

    std::vector<MapLayer> layers;
    std::vector<EntityInstance> entities;
    std::vector<TileProperties*> tileProperties;

    EntityDictionary properties;

    Vector2 startMarker;
    Vector2 endMarker;

private:
    std::vector<EntityProperty> _mapProperties;
};

