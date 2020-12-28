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
};


class BaseResource
{
public:
    virtual bool LoadFromFile(const ResourceSettings& settings) { return false; }

    template<typename TResource>
    TResource* Get();

    static BaseResource* GetDefaultResource() { return nullptr; }

    std::string name;
private:
};

template<typename TResource>
TResource* BaseResource::Get()
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

class NewResourceManager
{
public:
    void LoadResourceFromFile(const char* filePath, const char* resourceName);

    void SetBaseAssetPath(const char* path)
    {
        _baseAssetPath = path;
    }

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

    static NewResourceManager* GetInstance();
    bool EnableDefaultAssets() const { return _replaceWithDefault; }

private:
    void AddResource(BaseResource* resource, const char* name);

    std::unordered_map<unsigned int, std::unique_ptr<BaseResource>> _resourcesByStringId;
    bool _replaceWithDefault = true;
    std::filesystem::path _baseAssetPath;
};

template<typename TResource>
TResource* GetResource(const char* name)
{
    auto instance = NewResourceManager::GetInstance();
    auto resource = instance->GetResourceByName(name);
    if (resource != nullptr)
    {
        return resource->Get<TResource>();
    }
    else if (instance->EnableDefaultAssets())
    {
        if (auto defaultResource = TResource::GetDefaultResource())
        {
            return defaultResource->template Get<TResource>();
        }
    }

    FatalError("Missing resource %s of type %s\n", name, typeid(TResource).name());
}