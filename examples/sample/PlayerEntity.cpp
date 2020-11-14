#include "PlayerEntity.hpp"


#include "InputService.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"

void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    rigidBody = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    auto box = rigidBody->CreateBoxCollider(Dimensions());

    box->SetDensity(1);
    box->SetFriction(0);

    net = AddComponent<NetComponent>();

    scene->SendEvent(PlayerAddedToGame(this));

    scene->GetService<InputService>()->players.push_back(this);
}

void PlayerEntity::OnEvent(const IEntityEvent& ev)
{
    if(ev.Is<SpawnedOnClientEvent>())
    {
        //if (net->netId == scene->replicationManager.localNetId)
        {
            scene->GetCameraFollower()->FollowEntity(this);
            scene->GetCameraFollower()->CenterOn(Center());
            scene->GetService<InputService>()->activePlayer = this;
            net->isClientPlayer = true;
            scene->replicationManager.localPlayer = this;
        }
    }
    else if(auto flowFieldReady = ev.Is<FlowFieldReadyEvent>())
    {
        net->flowField = flowFieldReady->result;
    }
}

void PlayerEntity::OnDestroyed()
{
    scene->SendEvent(PlayerRemovedFromGame(this));
}

void PlayerEntity::Render(Renderer* renderer)
{
    auto position = Center();

    renderer->RenderRectangle(Rectangle(position - Dimensions() / 2, Dimensions()), Color::CornflowerBlue(), -0.99);
}

void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}
