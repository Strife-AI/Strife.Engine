#include "NetworkPhysics.hpp"


#include "../../examples/sample/InputService.hpp"
#include "Components/NetComponent.hpp"
#include "Scene/Scene.hpp"

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
    // Update position based on prediction
    self->owner->SetCenter(self->positionAtStartOfCommand);

    // Lock other players
    for (auto player : scene->replicationManager.components)
    {
        if (player != self)
        {
            player->rigidBody->body->SetLinearVelocity(b2Vec2(0, 0));
        }
    }

    // Update client side prediction
    for (auto& command : self->net->commands)
    {
        if ((int)command.id > (int)self->net->lastServedExecuted)
        {
            for (int i = 0; i < (int)command.fixedUpdateCount; ++i)
            {
                self->SetMoveDirection(GetDirectionFromKeyBits(command.keys) * 300);
                scene->ForceFixedUpdate();
            }
        }
    }
}