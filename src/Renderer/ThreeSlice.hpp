#pragma once

#include "Sprite.hpp"
#include "Memory/ResourceManager.hpp"

enum class ThreeSliceMode
{
    Stretch,
    Repeat
};

class ThreeSlice
{
public:
    ThreeSlice(Resource<Sprite> sprite, Vector2 cornerSize, ThreeSliceMode mode = ThreeSliceMode::Repeat)
        : _sprite(sprite),
        _cornerSize(cornerSize),
        _mode(mode)
    {

    }

    Sprite* GetSprite() const { return _sprite.Value(); }
    Vector2 CornerSize() const { return _cornerSize; }
    ThreeSliceMode Mode() const { return _mode; }

private:
    Resource<Sprite> _sprite;
    Vector2 _cornerSize;
    ThreeSliceMode _mode;
};