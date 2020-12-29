#pragma once

#include "Math/Rectangle.hpp"

class Texture;

class Sprite
{
public:
    explicit Sprite()
        : _texture(nullptr)
    {
    }

    explicit Sprite(Texture* texture, const Rectangle& bounds);
    explicit Sprite(Texture* texture, const Rectangle& bounds, const Rectangle& uvBounds);

    const Rectangle Bounds() const { return _bounds; }
    Texture* GetTexture() const { return _texture; }
    const Rectangle UVBounds() const { return _uvBounds; }

private:
    Texture* _texture;
    Rectangle _bounds;
    Rectangle _uvBounds;
};
