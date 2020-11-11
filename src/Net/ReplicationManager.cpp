#include "ReplicationManager.hpp"

#include <slikenet/BitStream.h>


#include "NetworkManager.hpp"
#include "Scene/Scene.hpp"

enum class MessageType : uint8
{
    SpawnEntity,
    EntitySnapshot
};

struct ReadWriteBitStream
{
    ReadWriteBitStream(SLNet::BitStream& stream_, bool isReading_)
        : stream(stream_),
        isReading(isReading_)
    {
        
    }

    template<typename T>
    ReadWriteBitStream& Add(T& out)
    {
        if (isReading) stream.Read(out);
        else stream.Write(out);

        return *this;
    }

    ReadWriteBitStream& Add(Vector2& out)
    {
        if (isReading)
        {
            stream.Read(out.x);
            stream.Read(out.y);
        }
        else
        {
            stream.Write(out.x);
            stream.Write(out.y);
        }

        return *this;
    }

    SLNet::BitStream& stream;
    bool isReading;
};

struct SpawnEntityMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream
            .Add(netId)
            .Add(type.key)
            .Add(position);
    }

    uint16 netId;
    StringId type;
    Vector2 position;
};

struct EntityUpdateMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream.Add(netId).Add(position);
    }

    uint16 netId;
    Vector2 position;
};

struct EntitySnapshotMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream
            .Add(lastServerSequence)
            .Add(lastExecuted)
            .Add(totalEntities);

        for(int i = 0; i < totalEntities; ++i)
        {
            entities[i].ReadWrite(stream);
        }
    }

    static constexpr int MaxEntities = 50;

    int lastServerSequence;
    int lastExecuted;

    uint8 totalEntities;
    EntityUpdateMessage entities[50];
};

struct PlayerCommandMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream.Add(keys).Add(fixedUpdateCount);
    }

    uint8 keys;
    uint8 fixedUpdateCount;
};

struct ClientUpdateRequestMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream.Add(commandCount);

        if (commandCount > 0)
        {
            stream.Add(firstCommandId);

            for (int i = 0; i < commandCount; ++i)
            {
                commands[i].ReadWrite(stream);
            }
        }
    }

    static constexpr int MaxCommands = 60;

    uint8 commandCount;
    uint32 firstCommandId;
    PlayerCommandMessage commands[MaxCommands];
};

void ReplicationManager::UpdateClient(SLNet::BitStream& stream)
{
    uint8 messageType;
    stream.Read(messageType);

    ReadWriteBitStream rw(stream, true);

    switch((MessageType)messageType)
    {
    case MessageType::SpawnEntity:
        ProcessSpawnEntity(rw);
        break;

    case MessageType::EntitySnapshot:
        ProcessEntitySnapshotMessage(rw);
        break;

    default:
        break;
    }
}

void ReplicationManager::DoClientUpdate(float deltaTime, NetworkManager* networkManager)
{
    _sendUpdateTimer -= _scene->deltaTime;

    // Time to send new update to server with missing commands
    if (_sendUpdateTimer <= 0)
    {
        _sendUpdateTimer = 1.0 / 30;

        auto self = localPlayer.GetValueOrNull()->GetComponent<NetComponent>();

        // Garbage collect old commands that the server already has
        {
            while (!self->commands.IsEmpty() && self->commands.Peek().id < self->lastServedExecuted)
            {
                self->commands.Dequeue();
            }
        }

        ClientUpdateRequestMessage request;
        int commandCount = 0;

        for (auto& command : self->commands)
        {
            if (command.id > self->lastServerSequenceNumber)
            {
                if(commandCount == 0)
                {
                    request.firstCommandId = command.id;
                }

                request.commands[commandCount].keys = command.keys;
                request.commands[commandCount].fixedUpdateCount = command.fixedUpdateCount;

                if(++commandCount == ClientUpdateRequestMessage::MaxCommands)
                {
                    break;
                }
            }
        }

        request.commandCount = commandCount;

        networkManager->SendPacketToServer([&](SLNet::BitStream& message)
        {
            message.Write(PacketType::UpdateRequest);
            ReadWriteBitStream stream(message, false);
            request.ReadWrite(stream);
        });
    }
}

void ReplicationManager::ProcessSpawnEntity(ReadWriteBitStream& stream)
{
    SpawnEntityMessage message;
    message.ReadWrite(stream);

    EntityDictionary properties
    {
        { "type", message.type },
        { "position", message.position },
    };

    auto entity = _scene->CreateEntity(properties);
    auto netComponent = entity->GetComponent<NetComponent>();
    netComponent->netId = message.netId;
}

void ReplicationManager::ProcessEntitySnapshotMessage(ReadWriteBitStream& stream)
{
    EntitySnapshotMessage message;
    message.ReadWrite(stream);
}
