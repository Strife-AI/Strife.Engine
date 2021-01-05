#pragma once

#include "ResourceManager.hpp"
#include "Scene/MapSegment.hpp"

struct TilemapResource : BaseResource
{
    bool LoadFromFile(const ResourceSettings& settings) override;
    MapSegment mapSegment;
};