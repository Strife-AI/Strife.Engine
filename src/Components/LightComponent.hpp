#pragma once

#include "Scene/EntityComponent.hpp"
#include "Engine.hpp"
#include "Scene/Entity.hpp"
#include "Scene/IEntityEvent.hpp"
#include "Renderer/Renderer.hpp"

template<typename TLight>
struct LightComponent : ComponentTemplate<LightComponent<TLight>>, TLight
{
	void OnAdded() override;
	void OnRemoved() override;
	void ReceiveEvent(const IEntityEvent& ev) override;

	Vector2 offsetFromCenter;
};

template<typename TLight>
void LightComponent<TLight>::OnAdded()
{
	if (this->GetScene()->isServer)
	{
		return;
	}

	auto renderer = this->GetScene()->GetEngine()->GetRenderer();

	if (renderer != nullptr)
	{
		renderer->GetLightManager()->AddLight(this);
	}
}

template<typename TLight>
void LightComponent<TLight>::OnRemoved()
{
	if (this->GetScene()->isServer)
	{
		return;
	}

	auto renderer = this->GetScene()->GetEngine()->GetRenderer();

	if (renderer != nullptr)
	{
		renderer->GetLightManager()->RemoveLight(this);
	}
}

template<typename TLight>
void LightComponent<TLight>::ReceiveEvent(const IEntityEvent& ev)
{
	if (ev.Is<EntityMovedEvent>())
	{
		this->position = this->owner->Center() + offsetFromCenter;
	}
}
