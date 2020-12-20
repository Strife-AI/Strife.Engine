#include "EntityComponent.hpp"
#include "Entity.hpp"
#include "Scene.hpp"

Scene* IEntityComponent::GetScene() const
{
    return owner->scene;
}

void IEntityComponent::Render(Renderer* renderer)
{
	componentFlags.ResetFlag(EntityComponentFlags::EnableRender);
	FlagsChanged();
}

void IEntityComponent::FlagsChanged()
{
	owner->scene->GetComponentManager().ScheduleToUpdateInterfaces(this);
}

void IEntityComponent::Update(float deltaTime)
{
	componentFlags.ResetFlag(EntityComponentFlags::EnableUpdate);
	FlagsChanged();
}

void IEntityComponent::FixedUpdate(float deltaTime)
{
	componentFlags.ResetFlag(EntityComponentFlags::EnableFixedUpdate);
	FlagsChanged();
}

IEntityComponent::IEntityComponent()
	: componentFlags({ EntityComponentFlags::EnableFixedUpdate, EntityComponentFlags::EnableUpdate, EntityComponentFlags::EnableRender })
{

}

void IEntityComponent::Register()
{
	GetScene()->GetComponentManager().Register(this);
}

void EntityComponentManager::Register(IEntityComponent* component)
{
	if (component->componentFlags.HasFlag(EntityComponentFlags::EnableUpdate)) updatables.insert(component);
	else updatables.erase(component);

	if (component->componentFlags.HasFlag(EntityComponentFlags::EnableRender)) renderables.insert(component);
	else renderables.erase(component);

	if (component->componentFlags.HasFlag(EntityComponentFlags::EnableFixedUpdate)) fixedUpdatables.insert(component);
	else fixedUpdatables.erase(component);
}

void EntityComponentManager::Unregister(IEntityComponent* component)
{
	renderables.erase(component);
	updatables.erase(component);
	fixedUpdatables.erase(component);
}

void EntityComponentManager::ScheduleToUpdateInterfaces(IEntityComponent* component)
{
	toBeUpdated.insert(component);
}

void EntityComponentManager::UpdateScheduledComponents()
{
	for(auto component : toBeUpdated)
	{
		Register(component);
	}

	toBeUpdated.clear();
}
