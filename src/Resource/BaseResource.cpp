#include "BaseResource.hpp"
#include "SpriteResource.hpp"
#include "TilemapResource.hpp"

void NewResourceManager::LoadResourceFromFile(const char* filePath, const char* resourceName)
{
    std::filesystem::path path = _baseAssetPath/std::filesystem::path(filePath);
    auto pathString = path.string();

    ResourceSettings settings;
    settings.path = pathString.c_str();
    settings.isHeadlessServer = false;
    settings.resourceName = resourceName;

    BaseResource* resource = nullptr;

    if (path.extension() == ".png") resource = new SpriteResource;
    else if (path.extension() == ".tmx") resource = new TilemapResource;

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

void NewResourceManager::AddResource(BaseResource* resource, const char* name)
{
    // TODO: check for duplicates
    _resourcesByStringId[StringId(name).key] = std::unique_ptr<BaseResource>(resource);
}

NewResourceManager* NewResourceManager::GetInstance()
{
    static NewResourceManager resourceManager;
    return &resourceManager;
}
