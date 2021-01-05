#pragma once

#include <utility>
#include <typeinfo>
#include <unordered_set>

#include "Memory/Flags.hpp"


enum class EntityComponentFlags
{
	EnableUpdate = 1,
	EnableFixedUpdate = 2,
	EnableRender = 4,
};

struct Entity;
class Scene;
class Renderer;

struct IEntityComponent
{
	IEntityComponent();
    virtual ~IEntityComponent() = default;

    virtual void OnAdded() { }
    virtual void OnRemoved() { }
    virtual void OnOwnerMoved() { }

    virtual void Render(Renderer* renderer);
    virtual void Update(float deltaTime);
    virtual void FixedUpdate(float deltaTime);
    virtual void ReceiveEvent(const struct IEntityEvent& ev) { };

    void Register();

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

private:
	void FlagsChanged();
};

template<typename TComponent>
struct ComponentTemplate : IEntityComponent
{
    std::pair<int, void*> GetMemoryBlock() override
    {
        return { (int)sizeof(TComponent), static_cast<TComponent*>(this) };
    }
};

struct EntityComponentManager
{
	void Register(IEntityComponent* component);
	void Unregister(IEntityComponent* component);
	void ScheduleToUpdateInterfaces(IEntityComponent* component);
	void UpdateScheduledComponents();

	std::unordered_set<IEntityComponent*> renderables;
	std::unordered_set<IEntityComponent*> updatables;
	std::unordered_set<IEntityComponent*> fixedUpdatables;
	std::unordered_set<IEntityComponent*> toBeUpdated;
};


#define DEFINE_COMPONENT(comeponentStructName_) struct comeponentStructName_ : ComponentTemplate<comeponentStructName_>