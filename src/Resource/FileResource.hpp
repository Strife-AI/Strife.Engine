#pragma once

#include <vector>
#include "ResourceManager.hpp"

struct FileResource : BaseResource
{
    bool LoadFromFile(const ResourceSettings& settings) override;

    std::vector<unsigned char>& Get()
    {
        return _bytes;
    }

private:
    std::vector<unsigned char> _bytes;
};