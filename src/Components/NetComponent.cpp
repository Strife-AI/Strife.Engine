#include "NetComponent.hpp"

#include "Net/ReplicationManager.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Net/ServerGame.hpp"

ISyncVar::ISyncVar(SyncVarUpdateFrequency frequency_)
    : frequency(frequency_)
{
    auto entity = Scene::entityUnderConstruction;
    if(entity == nullptr)
    {
        FatalError("Syncvars can only be added to an entity");
    }

    next = entity->syncVarHead;
    entity->syncVarHead = this;
}

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
