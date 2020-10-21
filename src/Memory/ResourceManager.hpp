#pragma once

#include <unordered_map>
#include <vector>

#include "StringId.hpp"
#include "System/Logger.hpp"

const int CONTENT_VERSION = 10;

class BinaryStreamReader;
struct AtlasAnimation;
class Engine;
struct MapSegment;
class Sprite;
class SpriteFont;
class NineSlice;

struct IResource
{
	virtual ~IResource() = default;
	virtual void Replace(void* data, int newGeneration) = 0;

	void (*cleanup)(IResource* resource);
	int generation;
};

template <typename TResource>
struct InternalResource : IResource
{
	InternalResource(TResource* data_, StringId id_, void (*cleanup)(IResource* resource))
		: data(data_),
	    id(id_)
	{
		this->cleanup = cleanup;
	}

	void Replace(void* data, int newGeneration) override
	{
		this->data = (TResource*)(data);
		this->generation = newGeneration;
	}

	TResource* data;
	StringId id;
};

template <typename TResource>
class Resource
{
public:
	Resource(InternalResource<TResource>* resource)
	    : _resource(resource)
	{
	    
	}

	Resource()
		: _resource(nullptr)
	{
	    
	}

	TResource* operator->()
	{
		return _resource->data;
	}

	TResource* operator->() const
	{
		return _resource->data;
	}

	TResource* Value() const
	{
	    if (_resource == nullptr)
	        return nullptr;

		return _resource->data;
	}

	StringId Id() const
	{
		return _resource->id;
	}

private:
	InternalResource<TResource>* _resource;
};

class ResourceManager
{
public:
	template <typename TResource>
	static Resource<TResource> GetResource(StringId id);

	template <typename TResource>
	static bool TryGetResource(StringId id, Resource<TResource>& outResource);

	template <typename TResource>
	static void AddResource(StringId id, TResource* resource, void (*cleanup)(IResource*) = nullptr)
	{
		if (_resources.count(id.key) != 0)
		{
			_resources[id.key]->Replace((void*)resource, currentGeneration);
		}
		else
		{
			AddResourceInternal(id, new InternalResource<TResource>(resource, id, cleanup));
		}
	}

	static bool HasResource(StringId id)
	{
		auto it = _resources.find(id.key);

		if(it == _resources.end())
		{
			return false;
		}
		else
		{
			return it->second->generation == currentGeneration;
		}
	}

	static void LoadResourceFile(BinaryStreamReader& fileReader);

	static bool TryGetResourcesByType(StringId type, std::vector<StringId>& resources);

	template<typename TResource>
	static std::vector<Resource<TResource>> GetResourcesOfType();

	static void Initialize(Engine* engine_)
	{
		engine = engine_;
	}

	static void Cleanup();

	static Engine* engine;

private:
	static std::unordered_map<unsigned int, IResource*> _resources;
	static std::unordered_map<unsigned int, std::vector<unsigned int>> _resourceLists;

	static IResource* GetResourceInternal(StringId id);
	static void AddResourceInternal(StringId id, IResource* resource);

	static int currentGeneration;
};

template <typename TResource>
Resource<TResource> ResourceManager::GetResource(StringId id)
{
	auto resource = GetResourceInternal(id);

	if (resource == nullptr)
	{
		FatalError("Missing resource: %s", id.ToString());
	}

	auto result = dynamic_cast<InternalResource<TResource>*>(resource);

	if (result == nullptr)
	{
		FatalError("Resource is wrong type: %s", id.ToString());
	}

	return Resource<TResource>(result);
}

template <typename TResource>
bool ResourceManager::TryGetResource(StringId id, Resource<TResource>& outResource)
{
	auto resource = GetResourceInternal(id);

	if (resource == nullptr)
	{
		return false;
	}

	auto result = dynamic_cast<InternalResource<TResource>*>(resource);

	if (result == nullptr)
	{
		return false;
	}
	outResource = Resource<TResource>(result);
	return true;
}

template <typename TResource>
std::vector<Resource<TResource>> ResourceManager::GetResourcesOfType()
{
	std::vector<Resource<TResource>> result;

	for(auto resource : _resources)
	{
	    if(auto internalResource = dynamic_cast<InternalResource<TResource>*>(resource.second))
	    {
			result.push_back(Resource<TResource>(internalResource));
	    }
	}

	return result;
}
