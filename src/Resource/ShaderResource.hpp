#pragma once

#include "GL/Shader.hpp"
#include "ResourceManager.hpp"

struct ShaderResource : ResourceTemplate<Shader>
{
    bool LoadFromFile(const ResourceSettings& settings) override;
};