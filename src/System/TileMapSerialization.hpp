#pragma once

#include <vector>
#include <string>
#include <map>
#include <Math/Polygon.hpp>

#include "BinaryStreamReader.hpp"
#include "Math/Rectangle.hpp"

class BinaryStreamWriter;

struct TilePropertiesDto
{
	TilePropertiesDto()
	{
	    
	}

	TilePropertiesDto(StringId spriteResource_, const Rectanglei& bounds_, const Polygoni& shape_)
	    : spriteResource(spriteResource_),
	    bounds(bounds_),
	    shape(shape_)
	{
	    
	}

	StringId spriteResource;
	Rectanglei bounds;

	Polygoni shape;
};

struct ImportTileProperties
{
	ImportTileProperties(StringId spriteResource_, uint32_t id_, const Rectanglei& bounds_, const Polygoni& shape_, int serializationId_)
		: spriteResource(spriteResource_),
		id(id_),
	    bounds(bounds_),
	    shape(shape_),
	    serializationId(serializationId_)
	{

	}

	TilePropertiesDto ToTilePropertiesDto()
	{
		return TilePropertiesDto(spriteResource, bounds, shape);
	}

	StringId spriteResource;
	uint32_t id;

	Rectanglei bounds;
	Polygoni  shape;

	int serializationId;
};

struct TileMapLayerDto
{
    StringId layerName;
	Vector2i dimensions;
	Vector2i tileSize;
	std::vector<int> tiles;
};

struct EntityInstanceDto
{
	EntityInstanceDto()
	{
	    
	}

    EntityInstanceDto(const std::map<std::string, std::string>& properties_)
        : properties(properties_)
	{

	}

    std::map<std::string, std::string> properties;
};

struct MapSegmentDto
{
    void Write(BinaryStreamWriter& writer) const;
    void Read(BinaryStreamReader& reader);

	std::vector<Rectanglei> rectangleColliders;
	std::vector<Polygoni> polygonalColliders;

	std::vector<TilePropertiesDto> tileProperties;
	std::vector<TileMapLayerDto> layers;
	std::vector<EntityInstanceDto> entities;

	std::map<std::string, std::string> properties;
};