#pragma once

#include "Renderer/Sprite.hpp"
#include "ResourceManager.hpp"

struct SpriteResource : BaseResource
{
    bool LoadFromFile(const ResourceSettings& settings);

    Sprite sprite;
};
