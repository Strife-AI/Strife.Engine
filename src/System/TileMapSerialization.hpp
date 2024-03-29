#pragma once

#include <vector>
#include <string>
#include <map>
#include <Math/Polygon.hpp>
#include <robin_hood.h>

#include "BinaryStreamReader.hpp"
#include "Math/Rectangle.hpp"
#include "Memory/StringId.hpp"

class BinaryStreamWriter;

struct TilePropertiesDto
{
	TilePropertiesDto()
	{
	    
	}

	TilePropertiesDto(const std::string& spriteResource_, const Rectanglei& bounds_, const Polygoni& shape_, std::map<std::string, std::string> properties)
	    : spriteResource(spriteResource_),
	    bounds(bounds_),
	    shape(shape_),
	    properties(properties)
	{
	    
	}

	std::string spriteResource;
	Rectanglei bounds;
	Polygoni shape;
	std::map<std::string, std::string> properties;
};

struct ImportTileProperties
{
	ImportTileProperties(const std::string& spriteResource_, uint32_t id_, const Rectanglei& bounds_, const Polygoni& shape_, int serializationId_,
	    std::map<std::string, std::string> properties)
		: spriteResource(spriteResource_),
		id(id_),
	    bounds(bounds_),
	    shape(shape_),
	    serializationId(serializationId_),
	    properties(properties)
	{

	}

	TilePropertiesDto ToTilePropertiesDto()
	{
		return TilePropertiesDto(spriteResource, bounds, shape, properties);
	}

	std::string spriteResource;
	uint32_t id;

	Rectanglei bounds;
	Polygoni  shape;

	int serializationId;
	std::map<std::string, std::string> properties;
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

    EntityInstanceDto(const robin_hood::unordered_flat_map<std::string, std::string>& properties_)
        : properties(properties_)
	{

	}

    robin_hood::unordered_flat_map<std::string, std::string> properties;
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