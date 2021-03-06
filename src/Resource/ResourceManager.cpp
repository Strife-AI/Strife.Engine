#include "ResourceManager.hpp"
#include "SpriteResource.hpp"
#include "TilemapResource.hpp"
#include "SpriteFontResource.hpp"
#include "ShaderResource.hpp"
#include "FileResource.hpp"
#include "Tools/Console.hpp"

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
    else if (type == ".shader") resource = new ShaderResource;
    else if (type == ".txt" || type == ".file") resource = new FileResource;

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
        resource->path = pathString;
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
            settings.isHeadlessServer = false;
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