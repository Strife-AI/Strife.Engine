#include "Sprite.hpp"
#include "Texture.hpp"

Sprite::Sprite(Texture* texture, const Rectangle& bounds)
    : _texture(texture),
    _bounds(bounds),
    _uvBounds(bounds.TopLeft() / texture->Size(), bounds.Size() / texture->Size())
{

}

Sprite::Sprite(Texture* texture, const Rectangle& bounds, const Rectangle& uvBounds)
    : _texture(texture),
    _bounds(bounds),
    _uvBounds(uvBounds.TopLeft() / texture->Size(), (uvBounds.Size()) / texture->Size())
{

}
