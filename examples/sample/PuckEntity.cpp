#include "PuckEntity.hpp"


#include "Engine.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Renderer/Renderer.hpp"

void PuckEntity::OnAdded(const EntityDictionary& properties)
{
    auto rb = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    auto box = rb->CreateBoxCollider({ 15, 15 });

    if(GetEngine()->GetNetworkManger()->IsClient())
    {
        rb->body->SetType(b2_kinematicBody);
    }

    box->SetRestitution(1);
    box->SetFriction(1);
    box->SetDensity(1);

    AddComponent<NetComponent>();
}

void PuckEntity::Render(Renderer* renderer)
{
    renderer->RenderRectangle(Bounds(), Color::Red(), -1);
}
