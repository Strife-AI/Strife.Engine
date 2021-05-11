#pragma once

#include <nlohmann/json.hpp>
#include <optional>

struct ResourceSettings
{
    const char* path;
    Engine* engine;
    const char* resourceName;
    const char* resourceType;
    std::optional<nlohmann::json> attributes;
};
