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

    uint32 netId;   // TODO: make uint16
    Vector2 position;
};

struct EntitySnapshotMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream
            .Add(lastServerSequence)
            .Add(lastServerExecuted)
            .Add(totalEntities);

        for(int i = 0; i < totalEntities; ++i)
        {
            entities[i].ReadWrite(stream);
        }
    }

    static constexpr int MaxEntities = 50;

    int lastServerSequence;
    int lastServerExecuted;

    uint32 totalEntities;   // TODO: make uint8
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
    ReadWriteBitStream rw(stream, true);

    while (stream.GetNumberOfUnreadBits() >= 8)
    {
        uint8 messageType;
        stream.Read(messageType);

        switch ((MessageType)messageType)
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
                request.commands[commandCount].fixedUpdateCount = command.fixedUpdateCount; // TODO: clamp

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

void ReplicationManager::ProcessMessageFromClient(SLNet::BitStream& message, SLNet::BitStream& response, NetComponent* client)
{
    ClientUpdateRequestMessage request;
    ReadWriteBitStream readMessage(message, true);
    request.ReadWrite(readMessage);

    if (client->lastServerSequenceNumber + 1 <= request.firstCommandId)
    {
        for (int i = 0; i < request.commandCount; ++i)
        {
            unsigned int currentId = request.firstCommandId + i;
            if (currentId > client->lastServerSequenceNumber)
            {
                PlayerCommand newCommand;
                newCommand.id = currentId;
                newCommand.keys = request.commands[i].keys;
                newCommand.fixedUpdateCount = request.commands[i].fixedUpdateCount;
                client->lastServerSequenceNumber = currentId;

                if (client->commands.IsFull())
                {
                    client->commands.Dequeue();
                }

                client->commands.Enqueue(newCommand);
            }
        }
    }

    // Send the current state of the world
    response.Write(PacketType::UpdateResponse);
    response.Write(MessageType::EntitySnapshot);

    EntitySnapshotMessage responseMessage;
    responseMessage.lastServerSequence = client->lastServerSequenceNumber;
    responseMessage.lastServerExecuted = client->lastServedExecuted;
    responseMessage.totalEntities = components.size();

    int i = 0;
    for (auto p : components)
    {
        responseMessage.entities[i].netId = p->netId;

        if (p == client)
        {
            responseMessage.entities[i].position = client->positionAtStartOfCommand;
        }
        else
        {
            responseMessage.entities[i].position = p->owner->Center(); //p->PositionAtFixedUpdateId(clientCommandStartTime, currentFixedUpdateId);
        }

        ++i;
    }

    ReadWriteBitStream responseStream(response, false);
    responseMessage.ReadWrite(responseStream);
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

    auto self = localPlayer.GetValueOrNull()->GetComponent<NetComponent>();

    self->lastServerSequenceNumber = message.lastServerSequence;
    self->lastServedExecuted = message.lastServerExecuted;

    for (int i = 0; i < message.totalEntities; ++i)
    {
        auto player = _componentsByNetId.count(message.entities[i].netId) != 0
            ? _componentsByNetId[message.entities[i].netId]
            : nullptr;

        auto lastCommandExecuted = self->GetCommandById(self->lastServedExecuted);

        if (player == nullptr)
        {
            _scene->SendEvent(PlayerConnectedEvent(message.entities[i].netId, message.entities[i].position));
        }
        else if (player != self)
        {
            PlayerSnapshot snapshot;
            snapshot.commandId = self->lastServedExecuted;
            snapshot.position = message.entities[i].position;
            snapshot.time = lastCommandExecuted->timeRecorded;
            player->AddSnapshot(snapshot);
        }
        else
        {
            self->positionAtStartOfCommand = message.entities[i].position;
        }
    }
}
