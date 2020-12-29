#include <SDL_image.h>
#include <memory>

#include "SpriteResource.hpp"
#include "System/Logger.hpp"
#include "Renderer/Texture.hpp"

struct TextureManager
{
    void AddTexture(Texture* texture)
    {
        textures.emplace_back(texture);
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

    auto texture = new Texture(surface);
    g_textureManager.AddTexture(texture);
    SDL_FreeSurface(surface);

    sprite = Sprite(texture, Rectangle(Vector2(0, 0), texture->Size()));

    return true;
}
