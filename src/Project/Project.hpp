#pragma once

#include <filesystem>
#include <unordered_map>
#include <Memory/StringId.hpp>
#include <Math/Vector2.hpp>

struct PrefabModel
{
    StringId type;
    std::vector<std::pair<std::string, std::string>> properties;
};

struct EntityModel
{
    StringId type;
    Vector2 positon;
    std::unordered_map<std::string, std::string> properties;
};

struct SceneModel
{
//    std::unordered_map<std::string, std::string> properties;
    std::vector<EntityModel> entities;
};

class Project
{
    explicit Project(const std::filesystem::path& path);
//    void ToJson(const Project& project, const std::filesystem::path& path);

    Project() = default;

    bool TryGetScene(StringId name, SceneModel* sceneModel);

private:
    std::unordered_map<StringId, SceneModel> scenes;
    std::unordered_map<StringId, PrefabModel> prefabs;
};