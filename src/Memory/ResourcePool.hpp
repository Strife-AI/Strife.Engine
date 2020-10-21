#pragma once

#include <new>

#include "BlockAllocator.hpp"
#include "FreeList.hpp"

template<typename T>
class ResourcePool;

template<typename T>
class ResourceHandle
{
public:
    ResourceHandle()
        : _resource(nullptr),
        _owner(nullptr)
    {
        
    }

    ResourceHandle(T* resource, ResourcePool<T>* owner)
        : _resource(resource),
        _owner(owner)
    {
        
    }

    ResourceHandle(const ResourceHandle& rh) = default;
    ResourceHandle& operator=(const ResourceHandle& rhs) = default;

    T* operator->()
    {
        return _resource;
    }

    const T* operator->() const
    {
        return _resource;
    }

    T* Value() { return _resource; }

    void Release();

private:
    T* _resource;
    ResourcePool<T>* _owner;
};

template<typename T>
class ResourcePool
{
public:
    ResourceHandle<T> Borrow();
    void Return(ResourceHandle<T>& resource);

private:
    static T* Allocate();

    FreeList<T> _freeResources;
};

template <typename T>
void ResourceHandle<T>::Release()
{
    if(_resource != nullptr)
    {
        _owner->Return(*this);
        _resource = nullptr;
    }
}

template <typename T>
ResourceHandle<T> ResourcePool<T>::Borrow()
{
    T* resource;

    if(_freeResources.HasFreeItem())
    {
        resource = _freeResources.Borrow();
    }
    else
    {
        resource = Allocate();
    }

    new (resource) T();

    return { resource, this };
}

template <typename T>
void ResourcePool<T>::Return(ResourceHandle<T>& resource)
{
    resource.Value()->~T();
    _freeResources.Return(resource.Value());
}

template <typename T>
T* ResourcePool<T>::Allocate()
{
    return static_cast<T*>(BlockAllocator::GetDefaultAllocator()->Allocate(sizeof(T)));
}
