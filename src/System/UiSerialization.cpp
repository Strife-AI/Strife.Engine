#include "UiSerialization.hpp"

#include "Serialization.hpp"

void NineSliceDto::Write(BinaryStreamWriter &writer) const
{
    ::Write(writer, topLeftCornerSize);
    ::Write(writer, bottomRightCornerSize);
    writer.WriteInt(spriteName.key);
    writer.WriteInt(centerSpriteName.key);
}

void NineSliceDto::Read(BinaryStreamReader &reader)
{
    ::Read(reader, topLeftCornerSize);
    ::Read(reader, bottomRightCornerSize);
    spriteName = StringId(reader.ReadInt());
    centerSpriteName = StringId(reader.ReadInt());
}

void ThreeSliceDto::Write(BinaryStreamWriter& writer) const
{
    ::Write(writer, topLeftCornerSize);
    writer.WriteInt(spriteName.key);
}

void ThreeSliceDto::Read(BinaryStreamReader& reader)
{
    ::Read(reader, topLeftCornerSize);
    spriteName = StringId(reader.ReadInt());
}
