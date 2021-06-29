#include <fstream>

#include "ResourceManager.hpp"
#include "SpriteResource.hpp"
#include "TilemapResource.hpp"
#include "SpriteFontResource.hpp"
#include "ShaderResource.hpp"
#include "FileResource.hpp"
#include "Tools/Console.hpp"
#include "ResourceSettings.hpp"
#include "System/FileSystem.hpp"
#include "SpriteAtlasResource.hpp"

ConsoleVar<std::string> assetPath("asset-path", "../assets", true);

void ResourceManager::LoadResourceFromFile(const ResourceSettings& settings)
{
    if (_resourcesByStringId.count(StringId(settings.resourceName)) != 0)
    {
        return;
    }

    auto resourceType = settings.resourceType;

    BaseResource* resource = nullptr;

    if (strcmp(resourceType, "sprite") == 0) resource = new SpriteResource;
    else if (strcmp(resourceType, "map") == 0) resource = new TilemapResource;
    else if (strcmp(resourceType, "sprite-font") == 0) resource = new SpriteFontResource;
    else if (strcmp(resourceType, "shader") == 0) resource = new ShaderResource;
    else if (strcmp(resourceType, "file") == 0) resource = new FileResource;
    else if (strcmp(resourceType, "atlas") == 0) resource = new SpriteAtlasResource;

    if (resource == nullptr)
    {
        FatalError("Unknown resource type for file %s", settings.path);
    }

    if (!resource->LoadFromFile(settings))
    {
        FatalError("Failed to load resource %s\n", settings.path);
    }
    else
    {
        resource->name = settings.resourceName;
        resource->path = settings.path;
        AddResource(resource, settings.resourceName);
    }
}

void ResourceManager::AddResource(BaseResource* resource, const char* name)
{
    // TODO: check for duplicates
    _resourcesByStringId[StringId(name).key] = std::unique_ptr<BaseResource>(resource);
}

ResourceManager* ResourceManager::GetInstance()
{
    static ResourceManager resourceManager;
    return &resourceManager;
}

std::string ResourceManager::GetBaseAssetPath() const
{
    return assetPath.Value();
}

void ResourceManager::LoadContentFile(const char* filePath)
{
    auto absolutePath = GetAssetPath(filePath);
    std::ifstream file(absolutePath);

    if (!file.is_open())
    {
        FatalError("Failed to open content file: %s", absolutePath.c_str());
    }

    nlohmann::json json = nlohmann::json::parse(file);

    auto resources = json["resources"].get<std::vector<nlohmann::json>>();

    for (const auto& resourceJson : resources)
    {
        auto type = resourceJson["type"].get<std::string>();
        auto name = resourceJson["name"].get<std::string>();
        auto path = GetAssetPath(resourceJson["path"].get<std::string>());
        std::optional<nlohmann::json> attributes;

        if (resourceJson.contains("data"))
        {
            auto& dataSection = resourceJson["data"];

            if (dataSection.is_string())
            {
                std::ifstream attributeFile(dataSection.get<std::string>());
                attributes = nlohmann::json::parse(attributeFile);
            }
            else if (dataSection.is_object())
            {
                attributes = dataSection;
            }
        }

        ResourceSettings settings;
        settings.resourceName = name.c_str();
        settings.path = path.string().c_str();
        settings.resourceType = type.c_str();
        settings.attributes = attributes;

        LoadResourceFromFile(settings);
    }
}

std::filesystem::path ResourceManager::GetAssetPath(const std::string& relativePath)
{
    return std::filesystem::path(assetPath.Value()) / std::filesystem::path(relativePath);
}

static void ReloadResources(ConsoleCommandBinder& binder)
{
    std::string resourceName;
    binder
        .Bind(resourceName, "resourceName")
        .Help("Reloads a resource");

    auto resource = ResourceManager::GetInstance()->GetResourceByName(resourceName.c_str());
    if (resource == nullptr)
    {
        binder.GetConsole()->Log("No such resource %s", resourceName.c_str());
    }
    else
    {
        if (resource->TryCleanup())
        {
            ResourceSettings settings;
            settings.resourceName = resource->name.c_str();
            settings.path = resource->path.c_str();
            if (!resource->LoadFromFile(settings))
            {
                binder.GetConsole()->Log("Failed to reload resource\n");
            }
        }
        else
        {
            binder.GetConsole()->Log("Resource does not support reloading\n");
        }
    }
}

ConsoleCmd _reloadCmd("reload", ReloadResources);