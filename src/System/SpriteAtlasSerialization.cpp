#include <string>

#include "BinaryStreamWriter.hpp"
#include "BinaryStreamReader.hpp"

#include "SpriteAtlasSerialization.hpp"
#include "Serialization.hpp"

void Write(BinaryStreamWriter& writer, const SpriteSheetDto& spriteSheetDto)
{
    writer.WriteInt(spriteSheetDto.spriteSheetName.key);
    writer.WriteInt(spriteSheetDto.rows);
    writer.WriteInt(spriteSheetDto.cols);
    ::Write(writer, spriteSheetDto.cellSize);
}

void Read(BinaryStreamReader& reader, SpriteSheetDto& outSpriteSheet)
{
    outSpriteSheet.spriteSheetName = StringId(reader.ReadInt());
    outSpriteSheet.rows = reader.ReadInt();
    outSpriteSheet.cols = reader.ReadInt();
    ::Read(reader, outSpriteSheet.cellSize);
}

void Write(BinaryStreamWriter& writer, const AtlasAnimationDto& atlasAnimationDto)
{
    writer.WriteInt(atlasAnimationDto.fps);
    WriteVector(writer, atlasAnimationDto.frames);
    writer.WriteInt(atlasAnimationDto.animationName.key);
}

void Read(BinaryStreamReader& reader, AtlasAnimationDto& outAtlasAnimation)
{
    outAtlasAnimation.fps = reader.ReadInt();
    ReadVector(reader, outAtlasAnimation.frames);
    outAtlasAnimation.animationName = StringId(reader.ReadInt());
}

void SpriteAtlasDto::Write(BinaryStreamWriter &writer) const
{
    writer.WriteInt(atlasType.key);
    ::Write(writer, topLeftCornerSize);
    ::Write(writer, spriteSheet);
    WriteVector(writer, animations);
}

void SpriteAtlasDto::Read(BinaryStreamReader &reader)
{
    atlasType = StringId(reader.ReadInt());
    ::Read(reader, topLeftCornerSize);
    ::Read(reader, spriteSheet);
    ReadVector(reader, animations);
}

void SpriteFontDto::Write(BinaryStreamWriter &writer) const
{
    writer.WriteInt(rows);
    writer.WriteInt(cols);
    writer.WriteInt(spriteName.key);
}

void SpriteFontDto::Read(BinaryStreamReader &reader)
{
    rows = reader.ReadInt();
    cols = reader.ReadInt();
    spriteName = StringId(reader.ReadInt());
}

