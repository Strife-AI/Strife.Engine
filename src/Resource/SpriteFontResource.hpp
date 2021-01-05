#pragma once

#include <optional>

#include "Renderer/SpriteFont.hpp"
#include "ResourceManager.hpp"

struct SpriteFontResource : BaseResource
{
    bool LoadFromFile(const ResourceSettings& settings) override;

    SpriteFont* GetFont() { return &spriteFont.value(); }

private:
    std::optional<SpriteFont> spriteFont;
};