#include "Serialization.hpp"

#include "TileMapSerialization.hpp"
#include "BinaryStreamWriter.hpp"

void Write(BinaryStreamWriter& writer, const Polygoni& shape)
{
    WriteVector(writer, shape.points);
}

void Read(BinaryStreamReader& reader, Polygoni& shape)
{
    ReadVector(reader,shape.points);
}

void Write(BinaryStreamWriter& writer, const TilePropertiesDto& properties)
{
	writer.WriteInt(StringId(properties.spriteResource));
	Write(writer, properties.bounds);
	Write(writer, properties.shape);
}

void Read(BinaryStreamReader& reader, TilePropertiesDto& outProperties)
{
	outProperties.spriteResource = std::to_string(reader.ReadInt());
	Read(reader, outProperties.bounds);
	Read(reader, outProperties.shape);
}

void Write(BinaryStreamWriter& writer, const TileMapLayerDto& layer)
{
	writer.WriteInt(layer.layerName.key);
	Write(writer, layer.tileSize);
	Write(writer, layer.dimensions);
	WriteVector(writer, layer.tiles);
}

void Read(BinaryStreamReader& reader, TileMapLayerDto& outLayer)
{
	outLayer.layerName = StringId(reader.ReadInt());
	Read(reader, outLayer.tileSize);
	Read(reader, outLayer.dimensions);
	ReadVector(reader, outLayer.tiles);
}

void Write(BinaryStreamWriter& writer, const EntityInstanceDto& entity)
{
    WriteMap(writer, entity.properties);
}

void Read(BinaryStreamReader& reader, EntityInstanceDto& outEntity)
{
    ReadMap(reader, outEntity.properties);
}

void MapSegmentDto::Write(BinaryStreamWriter& writer) const
{
    WriteVector(writer, polygonalColliders);
	WriteVector(writer, rectangleColliders);
	WriteVector(writer, tileProperties);
	WriteVector(writer, layers);
	WriteVector(writer, entities);

	WriteMap(writer, properties);
}

void MapSegmentDto::Read(BinaryStreamReader& reader)
{
    ReadVector(reader, polygonalColliders);
	ReadVector(reader, rectangleColliders);
	ReadVector(reader, tileProperties);
	ReadVector(reader, layers);
	ReadVector(reader, entities);

	ReadMap(reader, properties);
}


