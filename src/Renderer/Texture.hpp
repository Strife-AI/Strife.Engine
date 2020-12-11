#pragma once


#include <vector>
#include <Math/Vector2.hpp>

#include "RenderVertex.hpp"

struct Color;
struct SDL_Surface;

struct TextureRenderInfo
{
    std::vector<int> elements;

    unsigned int lastVisibleFrame = -1;
};

enum TextureFlags
{
    PartiallyTransparent = 1
};

enum class TextureFormat
{
    RGBA,
    RGB,
    Red,
};

enum class TextureDataType
{
    Float,
    UnsignedByte
};

class Texture
{
public:
    Texture()
        :  _id(-1)
    {

    }

    Texture(SDL_Surface* surface);
    Texture(Color color, int width, int height);
    Texture(Color* texels, int width, int height);
    Texture(const Vector4& color, int width, int height);
    Texture(TextureFormat format, TextureDataType type, int width, int height);

    void UseNearestNeighbor();
    void UseLinearFiltering();

    void EnableRepeat();

    ~Texture();

    unsigned int Id() const { return _id; }
    Vector2 Size() const { return _size; }
    void Bind() const;
    void Unbind() const;
    void Resize(int w, int h);

    TextureRenderInfo renderInfo;

    unsigned int flags = 0;

private:
    void CreateOpenGlTexture(int width, int height, void* data, TextureFormat format, TextureDataType type);

    unsigned int _id;
    Vector2 _size;
};

