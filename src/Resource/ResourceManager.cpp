#include "ResourceManager.hpp"
#include "SpriteResource.hpp"
#include "TilemapResource.hpp"
#include "SpriteFontResource.hpp"

void ResourceManager::LoadResourceFromFile(const char* filePath, const char* resourceName, const char* resourceType)
{
    std::filesystem::path path = _baseAssetPath/std::filesystem::path(filePath);
    auto pathString = path.string();

    ResourceSettings settings;
    settings.path = pathString.c_str();
    settings.isHeadlessServer = false;
    settings.resourceName = resourceName;

    BaseResource* resource = nullptr;

    std::string type = resourceType != nullptr ? std::string(resourceType) : path.extension().string();

    if (type == ".png") resource = new SpriteResource;
    else if (type == ".tmx") resource = new TilemapResource;
    else if (type == ".sfnt") resource = new SpriteFontResource;

    if (resource == nullptr)
    {
        FatalError("Unknown resource type for file %s", settings.path);
    }

    if (!resource->LoadFromFile(settings))
    {
        Log("Failed to load resource %s\n", settings.path);
    }
    else
    {
        resource->name = resourceName;
        AddResource(resource, resourceName);
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
