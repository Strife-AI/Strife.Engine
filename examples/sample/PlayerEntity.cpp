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
        if (net->ownerClientId == scene->replicationManager.localClientId)
        {
            scene->GetCameraFollower()->FollowMouse();
            scene->GetCameraFollower()->CenterOn(Center());
            scene->GetService<InputService>()->activePlayer = this;
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

 void PlayerEntity::FixedUpdate(float deltaTime)
 {
     auto client = net;

     if (client->flowField != nullptr)
     {
         Vector2 velocity;

         Vector2 points[4];
         client->owner->Bounds().GetPoints(points);

         bool useBeeLine = true;
         for (auto p : points)
         {
             RaycastResult result;
             if (scene->RaycastExcludingSelf(p, client->flowField->target, nullptr, result))
             {
                 useBeeLine = false;
                 break;
             }
         }

         if (useBeeLine)
         {
             velocity = (client->flowField->target - client->owner->Center()).Normalize() * 200;
         }
         else
         {
             velocity = client->flowField->GetFilteredFlowDirection(client->owner->Center() - Vector2(16, 16)) * 200;

         }

         velocity = client->owner->GetComponent<RigidBodyComponent>()->GetVelocity().SmoothDamp(
             velocity,
             client->acceleration,
             0.05,
             Scene::PhysicsDeltaTime);

         if ((client->owner->Center() - client->flowField->target).Length() < 200 * Scene::PhysicsDeltaTime)
         {
             velocity = { 0, 0 };
             client->flowField = nullptr;
         }

         client->owner->GetComponent<RigidBodyComponent>()->SetVelocity(velocity);
         Renderer::DrawDebugLine({ client->owner->Center(), client->owner->Center() + velocity, Color::Red() });
     }
 }

 void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}

 void PlayerEntity::DoNetSerialize(NetSerializer& serializer)
 {
    if(serializer.Add(health))
    {
        
    }

    serializer.Add(ammo);
 }
