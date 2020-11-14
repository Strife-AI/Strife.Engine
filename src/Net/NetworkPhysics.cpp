#include "NetworkPhysics.hpp"

#include "Renderer.hpp"
#include "Components/NetComponent.hpp"
#include "Scene/Scene.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Physics/PathFinding.hpp"

void NetworkPhysics::ReceiveEvent(const IEntityEvent& ev)
{
    if(ev.Is<FixedUpdateEvent>())
    {
        if (_isServer)  ServerFixedUpdate();
        else            ClientFixedUpdate();
    }
}

void NetworkPhysics::ServerFixedUpdate()
{
    ++currentFixedUpdateId;

    for (auto player : scene->replicationManager.components)
    {
        PlayerCommand* commandToExecute = nullptr;

        while (true)
        {
            for (auto& currentCommand : player->commands)
            {
                if (currentCommand.status == PlayerCommandStatus::Complete)
                {
                    continue;
                }
                else
                {
                    commandToExecute = &currentCommand;
                    break;
                }
            }

            if (commandToExecute != nullptr)
            {
                if (commandToExecute->status == PlayerCommandStatus::NotStarted)
                {
                    commandToExecute->fixedUpdateStartId = currentFixedUpdateId;
                    commandToExecute->status = PlayerCommandStatus::InProgress;
                    commandToExecute->positionAtStartOfCommand = player->owner->Center();

                    if(commandToExecute->moveToTarget)
                    {
                        auto pathFinder = scene->GetService<PathFinderService>();
                        pathFinder->RequestFlowField(player->owner->Center(), commandToExecute->target, player->owner);
                    }
                }

                if ((int)commandToExecute->fixedUpdateCount - 1 <= 0)
                {
                    commandToExecute->status = PlayerCommandStatus::Complete;

                    player->lastServedExecuted = commandToExecute->id;
                    player->positionAtStartOfCommand = player->owner->Center();

                }
                else
                {
                    commandToExecute->fixedUpdateCount--;
                }

                if (player->flowField != nullptr)
                {
                    auto velocity = player->flowField->GetFilteredFlowDirection(player->owner->Center() - Vector2(16, 16)) * 200;

                    velocity = player->owner->GetComponent<RigidBodyComponent>()->GetVelocity().SmoothDamp(
                        velocity,
                        player->acceleration,
                        0.05,
                        Scene::PhysicsDeltaTime);

                    player->owner->GetComponent<RigidBodyComponent>()->SetVelocity(velocity);
                    Renderer::DrawDebugLine({ player->owner->Center(), player->owner->Center() + velocity, Color::Red() });
                }

                if(player->wasted != 0)
                {
                    --player->wasted;
                    continue;
                }

                break;
            }
            else
            {
                //player->owner->GetComponent<RigidBodyComponent>()->SetVelocity({ 0, 0 });
                ++player->wasted;

                break;
            }
        }

        // TODO: use flow grid
    }
}

void NetworkPhysics::ClientFixedUpdate()
{
    for(auto net : scene->replicationManager.components)
    {
        net->owner->SetCenter(net->GetSnapshotPosition(scene->timeSinceStart - 0.1));
    }
}