#pragma once
#include "Sprite.hpp"

enum class NineSliceMode
{
    Stretch,
    Repeat
};

#if false
class NineSlice
{
public:
    NineSlice(
        Resource<Sprite> sprite,
        Resource<Sprite> centerSprite,
        Vector2 cornerSize,
        NineSliceMode mode = NineSliceMode::Stretch)
        : _sprite(sprite),
        _centerSprite(centerSprite),
        _cornerSize(cornerSize),
        _mode(mode)
    {
        
    }

    NineSlice() { }

    Sprite* GetSprite() const { return _sprite.Value(); }
    Sprite* GetCenterSprite() const { return _centerSprite.Value(); }
    Vector2 CornerSize() const { return _cornerSize; }
    NineSliceMode Mode() const { return _mode; }

private:
    Resource<Sprite> _sprite;
    Resource<Sprite> _centerSprite;
    Vector2 _cornerSize;
    NineSliceMode _mode;
};
#endif