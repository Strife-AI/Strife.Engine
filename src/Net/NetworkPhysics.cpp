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
                            MoveToEvent moveTo;
                            moveTo.position = commandToExecute->target;
                            player->owner->SendEvent(moveTo);
                        }
                    }
                    else if(commandToExecute->attackTarget)
                    {
                        NetComponent* player = scene->replicationManager.GetNetComponentById(commandToExecute->netId);

                        if (player != nullptr)
                        {
                            NetComponent* attack = scene->replicationManager.GetNetComponentById(commandToExecute->attackNetId);

                            if(attack != nullptr)
                            {
                                AttackEvent attackEvent;
                                attackEvent.entity = attack->owner;
                                player->owner->SendEvent(attackEvent);
                            }
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
        net->owner->SetCenter(net->GetSnapshotPosition(scene->relativeTime - 0.2));
    }
}