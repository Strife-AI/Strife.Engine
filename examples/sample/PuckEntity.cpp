#include "PuckEntity.hpp"


#include "Engine.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Math/Random.hpp"
#include "Renderer/Renderer.hpp"

void PuckEntity::OnAdded(const EntityDictionary& properties)
{
    auto rb = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    auto box = rb->CreateBoxCollider(Dimensions());

    box->SetFriction(1);

    if(GetEngine()->GetNetworkManger()->IsClient())
    {
        //rb->body->SetType(b2_kinematicBody);
    }
    else
    {
        
    }

    box->SetRestitution(1);
    box->SetFriction(1);
    box->SetDensity(10);
    rb->body->ResetMassData();

    AddComponent<NetComponent>();

    //ChangeDirection();
}

void PuckEntity::Render(Renderer* renderer)
{
    renderer->RenderRectangle(Bounds(), Color::Red(), -1);
}

void PuckEntity::ChangeDirection()
{
    auto rb = GetComponent<RigidBodyComponent>();
    rb->SetVelocity(Rand({ -1, -1 }, { 1, 1 }) * 100);
    StartTimer(5, [=] { ChangeDirection(); });
}
