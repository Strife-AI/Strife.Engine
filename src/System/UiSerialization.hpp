#pragma once

#include <Math/Vector2.hpp>

#include "BinaryStreamWriter.hpp"
#include "BinaryStreamReader.hpp"

struct NineSliceDto
{
    NineSliceDto()
    {

    }

    void Write(BinaryStreamWriter& writer) const;
    void Read(BinaryStreamReader& reader);

    StringId spriteName;
    StringId centerSpriteName;

    Vector2i topLeftCornerSize;
    Vector2i bottomRightCornerSize;
};

struct ThreeSliceDto
{
    ThreeSliceDto()
    {

    }

    void Write(BinaryStreamWriter& writer)  const;
    void Read(BinaryStreamReader& reader);

    StringId spriteName;
    Vector2i topLeftCornerSize;
};