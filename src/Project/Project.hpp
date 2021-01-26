#pragma once
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <Memory/StringId.hpp>
#include <Math/Vector2.hpp>

struct PrefabModel
{
    StringId type;
    std::unordered_map<std::string, std::string> properties;
};

struct EntityModel
{
    EntityModel() = default;
    std::string name;

    StringId type = "<unknown>"_sid;
    std::unordered_map<std::string, std::string> properties = std::unordered_map<std::string, std::string>();
};

struct SceneModel
{
    std::string sceneName;
//    std::unordered_map<std::string, std::string> properties;
    std::vector<EntityModel> entities = std::vector<EntityModel>();
};

class Project
{
public:
    Project() = default;
    explicit Project(const std::filesystem::path& path);

    std::unordered_map<unsigned int, SceneModel> scenes;

private:

    std::unordered_map<unsigned int, PrefabModel> prefabs;
};