#include "FireballEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/NetComponent.hpp"
#include "HealthBarComponent.hpp"

void FireballEntity::OnAdded(const EntityDictionary& properties)
{
    auto rb = AddComponent<RigidBodyComponent>(b2_dynamicBody);
    rb->CreateCircleCollider(Radius, true);

    AddComponent<NetComponent>();

    light.color = Color::Orange();
    light.maxDistance = Radius;
    light.intensity = 2;

    if(!scene->isServer) scene->GetLightManager()->AddLight(&light);

    rb->SetVelocity(properties.GetValueOrDefault("velocity", Vector2{ 0, 0 }));
}

void FireballEntity::OnDestroyed()
{
    if(!scene->isServer) scene->GetLightManager()->RemoveLight(&light);
}

void FireballEntity::ReceiveServerEvent(const IEntityEvent& ev)
{
    if(auto contactBegin = ev.Is<ContactBeginEvent>())
    {
        if(!visible.Value())
        {
            return;
        }

        auto other = contactBegin->other.OwningEntity();
        if(other->id == ownerId) return;
        if(contactBegin->other.IsTrigger()) return;

        auto healthBar = other->GetComponent<HealthBarComponent>(false);

        if(healthBar != nullptr)
        {
            healthBar->TakeDamage(5, this);
        }

        visible.SetValue(false);
        StartTimer(1, [=] { Destroy(); });
    }
}

void FireballEntity::Update(float deltaTime)
{
    if(!scene->isServer)
    {
        if(!visible.Value())
        {
            scene->GetLightManager()->RemoveLight(&light);
        }
    }

    light.position = Center();
}
