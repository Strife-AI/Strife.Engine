#include <SDL_image.h>
#include <memory>
#include <System/BinaryStreamReader.hpp>

#include "SpriteResource.hpp"
#include "System/Logger.hpp"
#include "Renderer/Texture.hpp"

struct TextureManager
{
    Texture* AddTexture(std::unique_ptr<Texture> texture)
    {
        auto ptr = texture.get();
        textures.emplace_back(std::move(texture));
        return ptr;
    }

    std::vector<std::unique_ptr<Texture>> textures;
};

static TextureManager g_textureManager;

bool SpriteResource::LoadFromFile(const ResourceSettings& settings)
{
    if (settings.isHeadlessServer)
    {
        return true;
    }

    auto surface = IMG_Load(settings.path);

    if (surface == nullptr)
    {
        Log("Failed to load image! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }

    auto texture = g_textureManager.AddTexture(std::make_unique<Texture>(surface));
    SDL_FreeSurface(surface);

    _resource.emplace(texture, Rectangle(Vector2(0, 0), texture->Size()));

    return true;
}

bool SpriteResource::WriteToBinary(const ResourceSettings& settings, BinaryStreamWriter& writer)
{
    return writer.TryWriteFile(settings.path);
}

bool SpriteResource::LoadFromBinary(BinaryStreamReader& reader)
{
    const unsigned char* data = reader.Data();
    auto ops = SDL_RWFromMem(const_cast<void*>(reinterpret_cast<const void*>(data)), reader.TotalSize());
    SDL_Surface* loadedSurface = IMG_LoadPNG_RW(ops);
    auto texture = g_textureManager.AddTexture(std::make_unique<Texture>(loadedSurface));
    SDL_FreeSurface(loadedSurface);

    _resource.emplace(texture, Rectangle(Vector2(0, 0), texture->Size()));

    return true;
}