#include "NetworkPhysics.hpp"


#include "../../examples/sample/InputService.hpp"
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

    for (auto player : scene->GetService<InputService>()->players)
    {
        PlayerCommand* commandToExecute = nullptr;

        for (auto& currentCommand : player->net->commands)
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
                commandToExecute->positionAtStartOfCommand = player->Center();
            }

            auto direction = GetDirectionFromKeyBits(commandToExecute->keys) * 300;
            player->SetMoveDirection(direction);

            if ((int)commandToExecute->fixedUpdateCount - 1 <= 0)
            {
                commandToExecute->status = PlayerCommandStatus::Complete;

                player->net->lastServedExecuted = commandToExecute->id;
                player->net->positionAtStartOfCommand = player->Center();

            }
            else
            {
                commandToExecute->fixedUpdateCount--;
            }
        }
    }
}

void NetworkPhysics::ClientFixedUpdate()
{

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