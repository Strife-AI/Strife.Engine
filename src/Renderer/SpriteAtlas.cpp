#include <cassert>

#include "SpriteAtlas.hpp"
#include "Sprite.hpp"
#include "Renderer.hpp"
#include "Resource/SpriteResource.hpp"

void SpriteAtlas::GetFrame(const int frameIndex, Sprite* outSprite) const
{
    assert(frameIndex < _rows * _cols);

    const int row = frameIndex / _cols;
    const int col = frameIndex - row * _cols;

    const Vector2 cell(col, row);

    *outSprite = Sprite(
        _atlas->Get().GetTexture(),
        Rectangle(
            cell * (_cellSize + Vector2(8, 8)),
            _cellSize));
}
