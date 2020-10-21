#include <memory>

#include <Renderer/GL/glcorearb.h>
#include <Renderer/GL/gl3w.h>
#include "Texture.hpp"
#include "Color.hpp"
#include "SDL2/SDL.h"
#include "System/Logger.hpp"

int scale = 4;

Texture::Texture(SDL_Surface* surface)
    : _size(Vector2(surface->w, surface->h))
{
    auto format = surface->format->BytesPerPixel == 4
        ? TextureFormat::RGBA
        : TextureFormat::RGB;

    if (format == TextureFormat::RGBA)
    {
        unsigned int* newPixels = new unsigned int[surface->w * surface->h * scale * scale];
        unsigned int* pixels = (unsigned int*)surface->pixels;

        int newW = surface->w * scale;

        for (int i = 0; i < surface->h * scale; ++i)
        {
            for (int j = 0; j < surface->w * scale; ++j)
            {
                newPixels[i * newW + j] = pixels[(i / scale) * surface->w + (j / scale)];
            }
        }

        CreateOpenGlTexture(surface->w * scale, surface->h * scale, newPixels, format, TextureDataType::UnsignedByte);

        delete[] newPixels;
    }
    else
    {
        CreateOpenGlTexture(surface->w, surface->h, surface->pixels, format, TextureDataType::UnsignedByte);
    }
}

Texture::Texture(Color color, int width, int height)
    : _size(Vector2(width, height))
{
    auto data = std::make_unique<unsigned int[]>(width * height);
    std::fill(data.get(), data.get() + width * height, color.PackRGBA());
    CreateOpenGlTexture(width, height, data.get(), TextureFormat::RGBA, TextureDataType::UnsignedByte);
}

Texture::Texture(Color* texels, int width, int height)
    : _size(Vector2(width, height))
{
    auto data = std::make_unique<unsigned int[]>(width * height);
    for(int i = 0; i < width * height; ++i)
    {
        data[i] = texels[i].PackRGBA();
    }

    CreateOpenGlTexture(width, height, data.get(), TextureFormat::RGBA, TextureDataType::UnsignedByte);
}

Texture::Texture(const Vector4& color, int width, int height)
    : _size(Vector2(width, height))
{
    auto data = std::make_unique<float[]>(width * height * 4);

    for(int i = 0; i < width * height * 4; i += 4)
    {
        data[i] = color.x;
        data[i + 1] = color.y;
        data[i + 2] = color.z;
        data[i + 3] = color.w;
    }

    CreateOpenGlTexture(width, height, data.get(), TextureFormat::RGBA, TextureDataType::Float);
}

Texture::Texture(TextureFormat format, TextureDataType dataType, int width, int height)
    : _size(Vector2(width, height))
{
    CreateOpenGlTexture(width, height, nullptr, format, dataType);
}

void Texture::UseNearestNeighbor()
{
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void Texture::UseLinearFiltering()
{
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Texture::EnableRepeat()
{
    Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

Texture::~Texture()
{
    glDeleteTextures(1, &_id);
}

void Texture::Bind() const
{
    glBindTexture(GL_TEXTURE_2D, _id);
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Resize(int w, int h)
{
    
}

static int TextureFormatToGL(TextureFormat format)
{
    switch(format)
    {
    case TextureFormat::RGBA: return GL_RGBA;
    case TextureFormat::RGB: return GL_RGB;
    case TextureFormat::Red: return GL_RED;
    default: FatalError("Unhandled texture format type");
    }
}

static int TextureDataTypeToGL(TextureDataType dataType)
{
    switch(dataType)
    {
    case TextureDataType::Float: return GL_FLOAT;
    case TextureDataType::UnsignedByte: return GL_UNSIGNED_BYTE;
    default: FatalError("Unhandled texture data type");
    }
}

void Texture::CreateOpenGlTexture(int width, int height, void* data, TextureFormat format, TextureDataType type)
{
    glGenTextures(1, &_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _id);

    int formatGl = TextureFormatToGL(format);
    int typeGl = TextureDataTypeToGL(type);
    glTexImage2D(GL_TEXTURE_2D, 0, formatGl, width, height, 0, formatGl, typeGl, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    UseLinearFiltering();

    if(format == TextureFormat::RGBA && type == TextureDataType::UnsignedByte)
    {
        unsigned int* intPtr = (unsigned int*)data;
        for(int i = 0; i < width * height; ++i)
        {
            int alpha = Color::FromPacked(intPtr[i]).a;
            if(alpha < 255)
            {
                flags |= PartiallyTransparent;
                break;
            }
        }
    }
}
