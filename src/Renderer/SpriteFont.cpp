#include <Resource/SpriteResource.hpp>
#include "SpriteFont.hpp"
#include "Sprite.hpp"

SpriteFont::SpriteFont(SpriteResource* fontSprite, int rows, int cols)
        : _characterAtlas(fontSprite, std::vector<AtlasAnimation>(), rows, cols, Vector2(0, 0), Vector2(16, 16)),
          _characterDimensions({ 16, 16})
{
    auto size = fontSprite->Get().Bounds().Size();
    //Assert((int)size.x % cols == 0, "Font sprite not multiple of character width");
    //Assert((int)size.y % rows == 0, "Font sprite not multiple of character height");
}

void SpriteFont::GetCharacter(int c, Sprite* outSprite) const
{
    _characterAtlas.GetFrame(c, outSprite);
}

Vector2i SpriteFont::MeasureStringWithNewlines(const char* str, float scale) const
{
    int longestStringLength = 0;
    int currentStringLength = 0;
    int numberOfLines = 1;

    while(*str != '\0')
    {
        if(*str == '\n')
        {
            ++numberOfLines;

            if (currentStringLength > longestStringLength)
            {
                longestStringLength = currentStringLength;
            }

            currentStringLength = 0;
        }
        else
        {
            ++currentStringLength;
        }

        ++str;
    }

    if (currentStringLength > longestStringLength)
    {
        longestStringLength = currentStringLength;
    }

    auto size = Vector2(longestStringLength, numberOfLines) * _characterDimensions * scale;

    return size.AsVectorOfType<int>();
}

Vector2i SpriteFont::CharacterDimension(float scale) const
{
    return (_characterDimensions * scale).AsVectorOfType<int>();
}
