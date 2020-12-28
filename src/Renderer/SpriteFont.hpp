#pragma once

#include "SpriteAtlas.hpp"

class SpriteFont
{
public:
    SpriteFont(SpriteResource* fontSprite, int rows, int cols);

    void GetCharacter(int c, Sprite* outSprite) const;
    Vector2i MeasureStringWithNewlines(const char* str, float scale) const;
    Vector2i CharacterDimension(float scale) const;

private:
    SpriteAtlas _characterAtlas;
    Vector2 _characterDimensions;
};

