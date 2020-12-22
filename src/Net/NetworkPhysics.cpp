#include "NetworkPhysics.hpp"

#include "Renderer.hpp"
#include "ReplicationManager.hpp"
#include "Components/NetComponent.hpp"
#include "Scene/Scene.hpp"
#include "Components/RigidBodyComponent.hpp"

void UpdateVarsToTime(ISyncVar* head, float time)
{
    for (auto var = head; var != nullptr; var = var->next)
    {
        var->SetCurrentValueToValueAtTime(time);
    }
}

ConsoleVar<float> g_jitterTime("jitter", 0.2);

void NetworkPhysics::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<FixedUpdateEvent>())
    {
        if (_isServer) ServerFixedUpdate();
    }
    else if (ev.Is<UpdateEvent>())
    {
        if (!_isServer)
        {
            float nowInPast = scene->relativeTime - g_jitterTime.Value();
            for (auto net : scene->replicationManager->components)
            {
                if (net->markedForDestruction && nowInPast >= net->destroyTime)
                {
                    net->owner->Destroy();
                }

                UpdateVarsToTime(net->owner->syncVarHead, nowInPast);
                net->owner->UpdateSyncVars();
            }
        }
    }
}

void NetworkPhysics::ServerFixedUpdate()
{
    ++currentFixedUpdateId;

    for (auto& client : scene->replicationManager->GetClients())
    {
        PlayerCommand* commandToExecute = nullptr;

        while (true)
        {
            for (auto& currentCommand : client.second.commands)
            {
                if (currentCommand->status == PlayerCommandStatus::Complete)
                {
                    continue;
                }
                else
                {
                    commandToExecute = currentCommand;
                    break;
                }
            }

            if (commandToExecute != nullptr)
            {
                if (commandToExecute->status == PlayerCommandStatus::NotStarted)
                {
                    commandToExecute->fixedUpdateStartId = currentFixedUpdateId;
                    commandToExecute->status = PlayerCommandStatus::InProgress;

                    auto& handler = scene->replicationManager->playerCommandHandler.handlerByMetadata[commandToExecute->metadata];
                    if (handler != nullptr)
                    {
                        handler(*commandToExecute);
                    }
                }

                if ((int)commandToExecute->fixedUpdateCount - 1 <= 0)
                {
                    commandToExecute->status = PlayerCommandStatus::Complete;

                    client.second.lastServerExecutedCommandId = commandToExecute->id;
                }
                else
                {
                    commandToExecute->fixedUpdateCount--;
                }

                if (client.second.fixedUpdateCountWithMissingCommand > 0)
                {
                    --client.second.fixedUpdateCountWithMissingCommand;
                    continue;
                }

                break;
            }
            else
            {
                ++client.second.fixedUpdateCountWithMissingCommand;

                break;
            }
        }
    }
}