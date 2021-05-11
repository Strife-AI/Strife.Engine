#pragma once

#include <System/BinaryStreamWriter.hpp>
#include "Renderer/Sprite.hpp"
#include "ResourceManager.hpp"

struct SpriteResource : ResourceTemplate<Sprite>
{
    bool LoadFromFile(const ResourceSettings& settings) override;
    bool WriteToBinary(const ResourceSettings& settings, BinaryStreamWriter& writer) override;
    bool LoadFromBinary(BinaryStreamReader& reader) override;
};
