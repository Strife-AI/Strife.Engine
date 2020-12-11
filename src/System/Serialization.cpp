#include "Serialization.hpp"

#include <map>
#include <vector>
#include <string>

#include "BinaryStreamWriter.hpp"
#include "BinaryStreamReader.hpp"

void Write(BinaryStreamWriter& writer, int value)
{
    writer.WriteInt(value);
}

void Read(BinaryStreamReader& reader, int& outValue)
{
    outValue = reader.ReadInt();
}

void Write(BinaryStreamWriter& writer, float value)
{
    writer.WriteFloat(value);
}

void Read(BinaryStreamReader& reader, float& outValue)
{
    outValue = reader.ReadFloat();
}

void Write(BinaryStreamWriter& writer, const std::string& value)
{
    writer.WriteInt(value.size());

    if (value == "")
    {
        return;
    }

    writer.WriteBlob(&value[0], sizeof(char) * value.size());
}

void Read(BinaryStreamReader& reader, std::string& outValue) {
    int strSize = reader.ReadInt();
    outValue.resize(strSize);

    reader.ReadBlob(&outValue[0], sizeof(char) * strSize);
}

void Write(BinaryStreamWriter& writer, const Vector2i& v)
{
    writer.WriteInt(v.x);
    writer.WriteInt(v.y);
}

void Read(BinaryStreamReader& reader, Vector2i& outV)
{
    outV.x = reader.ReadInt();
    outV.y = reader.ReadInt();
}

void Write(BinaryStreamWriter& writer, const Rectanglei& rect)
{
    Write(writer, rect.topLeft);
    Write(writer, rect.bottomRight);
}

void Read(BinaryStreamReader& reader, Rectanglei& outRect)
{
    Read(reader, outRect.topLeft);
    Read(reader, outRect.bottomRight);
}
