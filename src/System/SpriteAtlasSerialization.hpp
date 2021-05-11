#pragma once

#include <vector>
#include <Memory/StringId.hpp>
#include "Math/Vector2.hpp"
#include "Math/Vector2.hpp"
#include "Renderer/SpriteAtlas.hpp"

class BinaryStreamWriter;
class BinaryStreamReader;

struct SpriteSheetDto
{
    SpriteSheetDto()
    {

    }

    SpriteSheetDto(StringId spriteSheetName_, int rows_, int cols_, Vector2i cellSize_)
        : spriteSheetName(spriteSheetName_),
        rows(rows_),
        cols(cols_),
        cellSize(cellSize_)
    {

    }

    StringId spriteSheetName;
    int rows;
    int cols;
    Vector2i cellSize;
};

struct SpriteAtlasDto
{
    SpriteAtlasDto()
    {

    }

    void Write(BinaryStreamWriter& writer) const;
    void Read(BinaryStreamReader& reader);

    StringId atlasType = "none"_sid;
    Vector2i topLeftCornerSize = Vector2::Zero().AsVectorOfType<int>();

    SpriteSheetDto spriteSheet;
    std::vector<AtlasAnimation> animations;
};

struct SpriteFontDto
{
    SpriteFontDto()
    {

    }

    void Write(BinaryStreamWriter& writer) const;
    void Read(BinaryStreamReader& reader);

    StringId spriteName;
    int rows;
    int cols;
};
