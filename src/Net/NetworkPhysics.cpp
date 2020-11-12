#include "NetworkPhysics.hpp"
#include "Components/NetComponent.hpp"
#include "Scene/Scene.hpp"
#include "Components/RigidBodyComponent.hpp"

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
            }

            auto direction = GetDirectionFromKeyBits(commandToExecute->keys) * 300;
            player->owner->GetComponent<RigidBodyComponent>()->SetVelocity(direction);

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
        }
        else
        {
            player->owner->GetComponent<RigidBodyComponent>()->SetVelocity({ 0, 0 });
        }
    }
}

void NetworkPhysics::ClientFixedUpdate()
{
    for(auto net : scene->replicationManager.components)
    {
        if (!net->isClientPlayer)
        {
            net->owner->SetCenter(net->GetSnapshotPosition(scene->timeSinceStart - 0.1));
        }
    }
}

void NetworkPhysics::UpdateClientPrediction(NetComponent* self)
{
    auto selfRb = self->owner->GetComponent<RigidBodyComponent>();

    // Update position based on prediction
    self->owner->SetCenter(self->positionAtStartOfCommand);

    // Lock other players
    for (auto player : scene->replicationManager.components)
    {
        auto rb = player->owner->GetComponent<RigidBodyComponent>();

        if (player != self) 
        {
             rb->body->SetLinearVelocity(b2Vec2(0, 0));
        }
    }

    // Update client side prediction
    for (auto& command : self->commands)
    {
        if ((int)command.id > (int)self->lastServedExecuted)
        {
            for (int i = 0; i < (int)command.fixedUpdateCount; ++i)
            {
                selfRb->SetVelocity(GetDirectionFromKeyBits(command.keys) * 300);
                scene->ForceFixedUpdate();
            }
        }
    }
}