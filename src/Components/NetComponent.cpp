#include "NetComponent.hpp"

#include "Net/ReplicationManager.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Net/ServerGame.hpp"

void NetComponent::OnAdded()
{
    GetScene()->replicationManager->AddNetComponent(this);
}

void NetComponent::OnRemoved()
{
    GetScene()->replicationManager->RemoveNetComponent(this);
}

PlayerCommand* PlayerCommandHandler::DeserializeCommand(ReadWriteBitStream& stream)
{
    uint8_t commandId;
    stream.stream.Read(commandId);

    auto deserializer = deserializerById.find(commandId);
    if (deserializer == deserializerById.end())
    {
        Log(LogType::Error, "No command handler for command id %d\n", (int)commandId);
        return nullptr;
    }

    return deserializer->second(stream);
}

void PlayerCommandHandler::FreeCommand(PlayerCommand* playerCommand)
{
    playerCommand->Free(blockAllocator);
}