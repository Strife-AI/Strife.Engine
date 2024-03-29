#pragma once

#include <optional>

#include "Renderer/SpriteFont.hpp"
#include "ResourceManager.hpp"

struct SpriteFontResource : ResourceTemplate<SpriteFont>
{
    bool LoadFromFile(const ResourceSettings& settings) override;
};