#pragma once

#include <string>
#include <unordered_map>
#include <Memory/StringId.hpp>
#include <filesystem>
#include "System/Logger.hpp"

struct Engine;

struct ResourceSettings
{
    const char* path;
    bool isHeadlessServer;
    std::string assetSettings;
    Engine* engine;
    const char* resourceName;
};


struct BaseResource
{
public:
    virtual bool LoadFromFile(const ResourceSettings& settings) { return false; }

    template<typename TResource>
    TResource* As();

    static BaseResource* GetDefaultResource() { return nullptr; }

    std::string name;
};

template<typename T>
struct ResourceTemplate : BaseResource
{
    T& Get() { return _resource.value(); }

protected:
    std::optional<T> _resource;
};

template<typename TResource>
TResource* BaseResource::As()
{
    if (auto result = dynamic_cast<TResource*>(this))
    {
        return result;
    }
    else
    {
        FatalError("Resource %s is not of type %s\n", name.c_str(), typeid(*this).name());
    }
}

class ResourceManager
{
public:
    void LoadResourceFromFile(const char* filePath, const char* resourceName, const char* resourceType = nullptr);
    void AddResource(const char* name, BaseResource* resource)
    {
        _resourcesByStringId[StringId(name)] = std::unique_ptr<BaseResource>(resource);
    }

    void SetBaseAssetPath(const char* path)
    {
        _baseAssetPath = path;
    }

    auto GetBaseAssetPath() const { return _baseAssetPath; }

    BaseResource* GetResourceByStringId(StringId id)
    {
        auto resource = _resourcesByStringId.find(id.key);
        return resource != _resourcesByStringId.end()
            ? resource->second.get()
            : nullptr;
    }

    BaseResource* GetResourceByName(const char* name)
    {
        return GetResourceByStringId(StringId(name));
    }

    static ResourceManager* GetInstance();
    bool EnableDefaultAssets() const { return _replaceWithDefault; }

private:
    void AddResource(BaseResource* resource, const char* name);

    std::unordered_map<unsigned int, std::unique_ptr<BaseResource>> _resourcesByStringId;
    bool _replaceWithDefault = true;
    std::filesystem::path _baseAssetPath;
};

template<typename TResource>
TResource* GetResource(const char* name, bool isFatal = true)
{
    auto instance = ResourceManager::GetInstance();
    auto resource = instance->GetResourceByName(name);
    if (resource != nullptr)
    {
        return resource->As<TResource>();
    }
    else if (instance->EnableDefaultAssets())
    {
        if (auto defaultResource = TResource::GetDefaultResource())
        {
            return defaultResource->template As<TResource>();
        }
    }

    if (isFatal)
    {
        FatalError("Missing resource %s of type %s\n", name, typeid(TResource).name());
    }
    else
    {
        return nullptr;
    }
}

template<typename TResource>
TResource* GetResource(StringId id, bool isFatal = true)
{
    auto instance = ResourceManager::GetInstance();
    auto resource = instance->GetResourceByStringId(id);
    if (resource != nullptr)
    {
        return resource->As<TResource>();
    }
    else if (instance->EnableDefaultAssets())
    {
        if (auto defaultResource = TResource::GetDefaultResource())
        {
            return defaultResource->template As<TResource>();
        }
    }

    if (isFatal)
    {
        FatalError("Missing resource %s of type %s\n", id.ToString(), typeid(TResource).name());
    }
    else
    {
        return nullptr;
    }
}