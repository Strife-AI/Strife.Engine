#include <Memory/Util.hpp>
#include "PlayerEntity.hpp"
#include "InputService.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Net/ReplicationManager.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"
#include "torch/torch.h"
#include "MessageHud.hpp"

#include "CastleEntity.hpp"
#include "FireballEntity.hpp"

void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    if(!scene->isServer)
        scene->GetLightManager()->AddLight(&light);

    light.position = Center();
    light.color = Color(255, 255, 255, 255);
    light.maxDistance = 400;
    light.intensity = 0.6;

    health = AddComponent<HealthBarComponent>();
    health->offsetFromCenter = Vector2(0, -20);

    rigidBody = AddComponent<RigidBodyComponent>(b2_dynamicBody);

    auto box = rigidBody->CreateBoxCollider(Dimensions());

    box->SetDensity(1);
    box->SetFriction(0);

    net = AddComponent<NetComponent>();

    scene->SendEvent(PlayerAddedToGame(this));

    scene->GetService<InputService>()->players.push_back(this);

    // Setup network and sensors
    {
        auto nn = AddComponent<NeuralNetworkComponent<PlayerNetwork>>();
        nn->SetNetwork("nn");

        // Network only runs on server
        if (scene->isServer) nn->mode = NeuralNetworkMode::Deciding;

        auto gridSensor = AddComponent<GridSensorComponent<40, 40>>(Vector2(16, 16));

        // Called when:
        //  * Collecting input to make a decision
        //  * Adding a training sample
        nn->collectInput = [=](PlayerModelInput& input)
        {
            input.velocity = rigidBody->GetVelocity();
            gridSensor->Read(input.grid);
        };

        // Called when the decider makes a decision
        nn->receiveDecision = [=](PlayerDecision& decision)
        {

        };

        // Collects what decision the player made
        nn->collectDecision = [=](PlayerDecision& outDecision)
        {
            outDecision.action = PlayerAction::Down;
        };
    }
}

void PlayerEntity::ReceiveEvent(const IEntityEvent& ev)
{

}

void PlayerEntity::ReceiveServerEvent(const IEntityEvent& ev)
{
    if (auto flowFieldReady = ev.Is<FlowFieldReadyEvent>())
    {
        net->flowField = flowFieldReady->result;
        net->acceleration = { 0, 0 };
    }
    else if (auto moveTo = ev.Is<MoveToEvent>())
    {
        scene->GetService<PathFinderService>()->RequestFlowField(Center(), moveTo->position, this);
        state = PlayerState::Moving;
    }
    else if (auto attack = ev.Is<AttackEvent>())
    {
        attackTarget = attack->entity;
        updateTargetTimer = 0;
        state = PlayerState::Attacking;
    }
    else if (auto outOfHealth = ev.Is<OutOfHealthEvent>())
    {
        Destroy();

        auto& selfName = scene->replicationManager->GetClient(outOfHealth->killer->GetComponent<NetComponent>()->ownerClientId).clientName;
        auto& otherName = scene->replicationManager->GetClient(net->ownerClientId).clientName;

        scene->SendEvent(BroadcastToClientMessage(selfName + " killed " + otherName + "'s bot!"));

        for(auto spawn : scene->GetService<InputService>()->spawns)
        {
            if(spawn->net->ownerClientId == net->ownerClientId)
            {
                auto server = GetEngine()->GetServerGame();

                spawn->StartTimer(10, [=]
                {
                    spawn->SpawnPlayer();
                    server->ExecuteRpc(spawn->net->ownerClientId, MessageHudRpc("Your bot has respawned at your base"));
                });

                break;
            }
        }
    }
}

void PlayerEntity::OnDestroyed()
{
    RemoveFromVector(scene->GetService<InputService>()->players, this);

    if(!scene->isServer)
        scene->GetLightManager()->RemoveLight(&light);
}

void PlayerEntity::Render(Renderer* renderer)
{
    light.position = Center();

    auto position = Center();

    Color c[5] = {
        Color::CornflowerBlue(),
        Color::Green(),
        Color::Orange(),
        Color::HotPink(),
        Color::Yellow()
    };

    auto color = c[net->ownerClientId];

    renderer->RenderRectangle(Rectangle(position - Dimensions() / 2, Dimensions()), color, -0.99);

    FontSettings font;
    font.spriteFont = ResourceManager::GetResource<SpriteFont>("console-font"_sid);
    font.scale = 0.75;

    auto& name = scene->replicationManager->GetClient(net->ownerClientId).clientName;
    Vector2 size = font.spriteFont->MeasureStringWithNewlines(name.c_str(), 0.75).AsVectorOfType<float>();

    renderer->RenderString(font, name.c_str(), Center() - Vector2(0, 32) - size / 2, -1);


//    for(auto p : PositionData().snapshots)
//    {
//        renderer->RenderCircle(p.
//        value, 2, Color::Red(), -1);
//    }

    //if(net->netId == )
//    printf("========================================\n");
//    Vector2 prev(0, 0);
//
//    for(auto it = PositionData().snapshots.begin(); it != PositionData().snapshots.end(); ++it)
//    {
//        auto next = it;
//        ++next;
//
//        if(next == PositionData().snapshots.end()) break;
//
//        float diff = (*next).time - (*it).time;
//
//        float length = ((*next).value - (*it).value).Length();
//
//        prev = (*it).value;
//
//        printf("time: %f, position: %f %f\n", (*it).time, (*it).value.x, (*it).value.y);
//    }
}

void PlayerEntity::ServerFixedUpdate(float deltaTime)
{
    auto client = net;

    attackCoolDown -= deltaTime;

    if(state == PlayerState::Attacking)
    {
        Entity* target;
        RaycastResult hitResult;
        if(attackTarget.TryGetValue(target)
            && (target->Center() - Center()).Length() < 200
            && scene->Raycast(Center(), target->Center(), hitResult)
            && hitResult.handle.OwningEntity() == target)
        {
            rigidBody->SetVelocity({ 0, 0 });
            auto dir = (target->Center() - Center()).Normalize();

            if (attackCoolDown <= 0)
            {
                EntityProperty properties[] = {
                    EntityProperty::EntityType<FireballEntity>(),
                    { "velocity", dir * 400 },
                    { "position", Center()  }
                };
                auto fireball = scene->CreateEntity({ properties });
                fireball->GetComponent<NetComponent>()->ownerClientId = net->ownerClientId;

                static_cast<FireballEntity*>(fireball)->ownerId = id;

                attackCoolDown = 1;
            }

            return;
        }
    }

    if (client->flowField != nullptr)
    {
        Vector2 velocity;

        Vector2 points[4];
        client->owner->Bounds().GetPoints(points);

        bool useBeeLine = true;
        for (auto p : points)
        {
            RaycastResult result;
            if (scene->Raycast(p, client->flowField->target, result, false, [=](auto collider) { return collider.OwningEntity() != this; })
                && (state != PlayerState::Attacking || result.handle.OwningEntity() != attackTarget.GetValueOrNull()))
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

        float dist = (client->flowField->target - client->owner->Center()).Length();
        if(dist > 20)
        {
            velocity = client->owner->GetComponent<RigidBodyComponent>()->GetVelocity().SmoothDamp(
                velocity,
                client->acceleration,
                0.05,
                Scene::PhysicsDeltaTime);
        }

        if(dist <= 1)
        {
            velocity = { 0, 0 };
            client->acceleration = { 0, 0 };
            client->flowField = nullptr;
        }

        client->owner->GetComponent<RigidBodyComponent>()->SetVelocity(velocity);
        //Renderer::DrawDebugLine({ client->owner->Center(), client->owner->Center() + velocity, useBeeLine ? Color::Red() : Color::Green() });
    }
    else
    {
        SetMoveDirection({ 0, 0 });
    }
}

void PlayerEntity::ServerUpdate(float deltaTime)
{
    if (state == PlayerState::Attacking)
    {
        Entity* target;
        if (attackTarget.TryGetValue(target))
        {
            updateTargetTimer -= deltaTime;
            if (updateTargetTimer <= 0)
            {
                scene->GetService<PathFinderService>()->RequestFlowField(Center(), target->Center(), this);
                updateTargetTimer = 2;
            }
            else if (net->flowField != nullptr)
            {
                net->flowField->target = target->Center();
            }
        }
        else
        {
            state = PlayerState::None;
            net->flowField = nullptr;
        }
    }
}

void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}

