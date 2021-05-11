#pragma once

#include <functional>
#include <utility>


#include "Color.hpp"
#include "Memory/StringId.hpp"
#include "Math/Vector2.hpp"

class Renderer;
class Sprite;
struct AtlasAnimation;
struct SpriteResource;

class SpriteAtlas
{
public:
    SpriteAtlas(SpriteResource* atlas,
        const std::vector<AtlasAnimation>& animations,
        int rows,
        int cols,
        Vector2 topLeftCornerSize,
        Vector2 cellSize,
        StringId atlasType = "none"_sid)
        : _atlas(atlas),
          _animations(animations),
          _rows(rows),
          _cols(cols),
          _atlasType(atlasType),
          _topLeftCornerSize(topLeftCornerSize),
          _cellSize(cellSize)
    {

    }

    void GetFrame(int frameIndex, Sprite* outSprite) const;
    const AtlasAnimation* GetAnimation(StringId name) const;

private:
    SpriteResource* _atlas;
    const std::vector<AtlasAnimation> _animations;
    const int _rows;
    const int _cols;
    const StringId _atlasType;
    const Vector2 _topLeftCornerSize;
    Vector2 _cellSize;
};

struct AtlasAnimation
{
    StringId name;
    int fps;
    std::vector<int> frames;
};