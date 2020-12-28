#pragma once

#include "Renderer/Sprite.hpp"
#include "BaseResource.hpp"

struct SpriteResource : BaseResource
{
    bool LoadFromFile(const ResourceSettings& settings);

    Sprite sprite;
};
