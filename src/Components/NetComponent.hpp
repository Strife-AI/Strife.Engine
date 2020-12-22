#pragma once

#include <vector>
#include <slikenet/BitStream.h>
#include <memory>
#include <Memory/BlockAllocator.hpp>
#include "Net/ServerGame.hpp"

#include "Math/Vector2.hpp"
#include "Memory/CircularQueue.hpp"
#include "Scene/EntityComponent.hpp"
#include "Scene/IEntityEvent.hpp"

enum class PlayerCommandStatus
{
    NotStarted,
    InProgress,
    Complete
};

class ReadWriteBitStream;

struct PlayerCommand
{
	struct Metadata { };

    void Serialize(ReadWriteBitStream& stream)
    {
    	stream.Add(fixedUpdateCount);
    	DoSerialize(stream);
    }

    virtual void DoSerialize(class ReadWriteBitStream& stream) { }
    virtual void Free(BlockAllocator* blockAllocator) = 0;

	uint8_t fixedUpdateCount;
	unsigned int id;

    // Server only
    PlayerCommandStatus status = PlayerCommandStatus::NotStarted;
    int fixedUpdateStartId;
    Metadata* metadata;

};

template<typename TCommand>
struct PlayerCommandTemplate : PlayerCommand
{
	PlayerCommandTemplate()
	{
		this->metadata = &typeMetadata;
	}

	void Free(BlockAllocator* blockAllocator) override
	{
		blockAllocator->Free(static_cast<TCommand*>(this), sizeof(TCommand));
	}

	static Metadata typeMetadata;
};

template<typename TCommand>
PlayerCommand::Metadata PlayerCommandTemplate<TCommand>::typeMetadata{ };

#define DEFINE_COMMAND(structName_) struct structName_ : PlayerCommandTemplate<structName_>

DEFINE_COMMAND(DoNothingCommand)
{

};

struct PlayerCommandHandler
{
	PlayerCommandHandler(BlockAllocator* blockAllocator)
		: blockAllocator(blockAllocator)
	{

	}

	struct ScheduledCommand
	{
		SLNet::BitStream stream;
		unsigned int commandId;
	};

	PlayerCommandHandler()
	{
		RegisterCommandType<DoNothingCommand>(0);
	}

	template<typename TCommand>
	void RegisterCommandType(int id, std::function<void(const TCommand& command)> onReceived = nullptr);

	void AddCommand(PlayerCommand& command)
	{
		command.fixedUpdateCount = fixedUpdateCount;
		command.id = ++nextCommandSequenceNumber;
		fixedUpdateCount = 0;

		uint8_t id = commandIdByMetadata[command.metadata];

		if (localCommands.IsFull())
		{
			localCommands.Dequeue();
		}

		auto scheduledCommand = localCommands.Allocate();
		scheduledCommand->stream.Reset();
		scheduledCommand->commandId = command.id;

		ReadWriteBitStream rw(scheduledCommand->stream, false);
		rw.stream.Write(id);
		command.Serialize(rw);
	}

	PlayerCommand* DeserializeCommand(ReadWriteBitStream& stream);
	void FreeCommand(PlayerCommand* command);

	std::unordered_map<int, std::function<PlayerCommand*(ReadWriteBitStream& stream)>> deserializerById;
	std::unordered_map<PlayerCommand::Metadata*, int> commandIdByMetadata;
	std::unordered_map<PlayerCommand::Metadata*, std::function<void(const PlayerCommand&)>> handlerByMetadata;

	BlockAllocator* blockAllocator;
	CircularQueue<ScheduledCommand, 32> localCommands;
	unsigned int fixedUpdateCount = 0;
	unsigned int nextCommandSequenceNumber = 0;
};

template<typename TCommand>
void PlayerCommandHandler::RegisterCommandType(int id, std::function<void(const TCommand&)> onReceived)
{
	deserializerById[id] = [=](auto& stream)
	{
		TCommand* command = reinterpret_cast<TCommand*>(blockAllocator->Allocate(sizeof(TCommand)));

		new (command) TCommand();
		command->Serialize(stream);

		return command;
	};

	handlerByMetadata[&TCommand::typeMetadata] = [=](const auto& command)
	{
		onReceived(static_cast<const TCommand&>(command));
	};

	commandIdByMetadata[&TCommand::typeMetadata] = id;
}


DEFINE_EVENT(SpawnedOnClientEvent)
{

};

DEFINE_COMPONENT(NetComponent)
{
    void OnAdded() override;
    void OnRemoved() override;

    int netId = -2;
    int ownerClientId = -2;
    bool isMarkedForDestructionOnClient = false;

    float destroyTime = INFINITY;
    bool markedForDestruction = false;
};
