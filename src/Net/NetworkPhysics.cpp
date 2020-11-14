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

    for (auto& client : scene->replicationManager.GetClients())
    {
        PlayerCommand* commandToExecute = nullptr;

        while (true)
        {
            for (auto& currentCommand : client.second.commands)
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

                    if(commandToExecute->moveToTarget)
                    {
                        NetComponent* player = scene->replicationManager.GetNetComponentById(commandToExecute->netId);

                        if (player != nullptr)
                        {
                            auto pathFinder = scene->GetService<PathFinderService>();
                            pathFinder->RequestFlowField(player->owner->Center(), commandToExecute->target, player->owner);
                        }
                    }
                }

                if ((int)commandToExecute->fixedUpdateCount - 1 <= 0)
                {
                    commandToExecute->status = PlayerCommandStatus::Complete;

                    client.second.lastServedExecuted = commandToExecute->id;
                }
                else
                {
                    commandToExecute->fixedUpdateCount--;
                }

                if(client.second.wasted > 0)
                {
                    --client.second.wasted;
                    continue;
                }

                break;
            }
            else
            {
                ++client.second.wasted;

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
        net->owner->SetCenter(net->GetSnapshotPosition(scene->timeSinceStart - 0.2));
    }
}