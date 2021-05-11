#pragma once

#include "ResourceManager.hpp"
#include "Renderer/SpriteAtlas.hpp"

struct SpriteAtlasResource : ResourceTemplate<SpriteAtlas>
{
    bool LoadFromFile(const ResourceSettings& settings) override;

};