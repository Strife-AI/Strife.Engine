#include <fstream>
#include <nlohmann/json.hpp>
#include <Resource/ResourceManager.hpp>
#include "System/Logger.hpp"

#include "Project.hpp"

using namespace nlohmann;

json LoadJsonFromFile(const std::filesystem::path& path)
{
    std::fstream file(path.string());
    if (file.bad())
    {
        file.close();
        FatalError("Failed to open file %s", path.c_str());
    }

    json content;
    file >> content;
    file.close();

    return content;
}

Project::Project(const std::filesystem::path& path)
{
    auto content = LoadJsonFromFile(path);
    Project project;

    auto resourceManager = ResourceManager::GetInstance();

    // Populate with resources
    for (auto& resource : content["resources"].get<std::vector<json>>())
    {
        if (resource["type"].get<std::string>() == "prefab")
        {
            auto prefabFilePath = path.parent_path() / resource["path"].get<std::string>();
            auto prefabFileContent = LoadJsonFromFile(prefabFilePath);

            PrefabModel model;
            for (const auto& property : prefabFileContent["properties"].get<std::vector<json>>())
            {
                model.properties[property["name"].get<std::string>()] = property["value"].get<std::string>();
            }

            model.type = StringId(prefabFileContent["entityType"].get<std::string>());

            prefabs[StringId(prefabFileContent["name"].get<std::string>()).key] = model;
        }
        else
        {
            resourceManager->LoadResourceFromFile(
                resource["path"].get<std::string>().c_str(),
                resource["name"].get<std::string>().c_str());
        }
    }

    // Generate scene models
    for (auto& scene : content["scenes"].get<std::vector<json>>())
    {
        auto scenePath = path.parent_path() / scene["path"].get<std::string>();
        auto sceneContent = LoadJsonFromFile(scenePath);

        SceneModel sceneModel;
        for (auto& entity : sceneContent["entities"].get<std::vector<json>>())
        {
            EntityModel model;

            if (entity.contains("prefab")) // If it contains the prefab key, add all of the properties on the prefab onto the new entitymodel
            {
                if (prefabs.find(StringId(entity["prefab"].get<std::string>()).key) == prefabs.end())
                {
                    printf("Failed to find prefab!\n");
                }

                auto fabModel = prefabs[StringId(entity["prefab"].get<std::string>()).key];

                for (auto& property : fabModel.properties)
                {
                    model.properties[property.first] = property.second;
                }

                model.type = fabModel.type;
            }
            else
            {
                model.type = StringId(entity["type"].get<std::string>());
            }

            model.name = entity["name"].get<std::string>();

            if (entity.contains("properties"))
            {
                for (auto& property : entity["properties"].get<std::vector<json>>())
                {
                    model.properties[property["name"].get<std::string>()] = property["value"].get<std::string>();
                }
            }

            sceneModel.entities.push_back(model);
        }

        sceneModel.sceneName = sceneContent["name"].get<std::string>();
        scenes[StringId(sceneModel.sceneName).key] = sceneModel;
    }
}