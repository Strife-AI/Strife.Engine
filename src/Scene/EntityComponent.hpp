#pragma once
#include <utility>

#include "Memory/Flags.hpp"

enum class EntityComponentFlags
{
    ReceivesRenderEvents = 1,
    ReceivesUpdateEvents = 2,
    ReceivesFixedUpdatedEvents = 3,
    Enabled = 4
};

struct Entity;
class Scene;
class Renderer;

struct IEntityComponent
{
    virtual ~IEntityComponent() = default;

    virtual void OnAdded() { }
    virtual void OnRemoved() { }
    virtual void OnOwnerMoved() { }

    virtual void Render(Renderer* renderer) { }
    virtual void Update(float deltaTime) { }

    virtual std::pair<int, void*> GetMemoryBlock() = 0;

    template<typename TComponent>
    TComponent* Is()
    {
        if(typeid(*this) == typeid(TComponent))
        {
            return static_cast<TComponent*>(this);
        }
        else
        {
            return nullptr;
        }
    }

    Scene* GetScene() const;

    Flags<EntityComponentFlags> componentFlags;
    IEntityComponent* next = nullptr;
    Entity* owner;
    const char* name = "<default>";
};

template<typename TComponent>
struct ComponentTemplate : IEntityComponent
{
    std::pair<int, void*> GetMemoryBlock() override
    {
        return { (int)sizeof(TComponent), static_cast<TComponent*>(this) };
    }
};

#define DEFINE_COMPONENT(comeponentStructName_) struct comeponentStructName_ : ComponentTemplate<comeponentStructName_>