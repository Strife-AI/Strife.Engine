#include "SpriteFontResource.hpp"
#include "SpriteResource.hpp"

StringId AddTileSet(const std::string& fileName, const std::string& resourceName, Vector2 tileSize);

bool SpriteFontResource::LoadFromFile(const ResourceSettings& settings)
{
    auto spriteName = std::string(settings.resourceName) + "-sprite-font";
    AddTileSet(settings.path, spriteName, Vector2(16, 16));

    auto sprite = GetResource<SpriteResource>(spriteName.c_str());
    spriteFont.emplace(sprite, 16, 16);

    return true;
}
