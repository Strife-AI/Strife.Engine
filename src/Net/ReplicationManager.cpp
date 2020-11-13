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
            .Add(position)
            .Add(dimensions);
    }

    uint16 netId;
    StringId type;
    Vector2 position;
    Vector2 dimensions;
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
        stream.Add(keys).Add(fixedUpdateCount).Add(moveToTarget).Add(target);
    }

    uint8 keys;
    uint8 fixedUpdateCount;
    bool moveToTarget;
    Vector2 target;
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

WorldDiff::WorldDiff(const WorldState& before, const WorldState& after)
{
    for(auto& currentEntity : after.entities)
    {
        if(before.entities.count(currentEntity) == 0)
        {
            addedEntities.push_back(currentEntity);
        }
    }
}

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
    if (_sendUpdateTimer <= 0 || true)
    {
        _sendUpdateTimer += 1.0 / 10;

        Entity* playerEntity;
        if (localPlayer.TryGetValue(playerEntity))
        {
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
                    if (commandCount == 0)
                    {
                        request.firstCommandId = command.id;
                    }

                    request.commands[commandCount].keys = command.keys;
                    request.commands[commandCount].fixedUpdateCount = command.fixedUpdateCount; // TODO: clamp
                    request.commands[commandCount].target = command.target;
                    request.commands[commandCount].moveToTarget = command.moveToTarget;

                    if (++commandCount == ClientUpdateRequestMessage::MaxCommands)
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
        else
        {
            networkManager->SendPacketToServer([&](SLNet::BitStream& message)
            {
                message.Write(PacketType::UpdateRequest);
                ReadWriteBitStream stream(message, false);

                ClientUpdateRequestMessage request;
                request.commandCount = 0;
                request.firstCommandId = 0;
                request.ReadWrite(stream);
            });
        }
    }
}

void ReplicationManager::ProcessMessageFromClient(SLNet::BitStream& message, SLNet::BitStream& response, NetComponent* client, int clientId)
{
    ClientUpdateRequestMessage request;
    ReadWriteBitStream readMessage(message, true);
    request.ReadWrite(readMessage);

    if (client != nullptr)
    {
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
                    newCommand.moveToTarget = request.commands[i].moveToTarget;
                    newCommand.target = request.commands[i].target;
                    client->lastServerSequenceNumber = currentId;

                    if (client->commands.IsFull())
                    {
                        client->commands.Dequeue();
                    }

                    client->commands.Enqueue(newCommand);
                }
            }
        }
    }

    ReadWriteBitStream responseStream(response, false);

    response.Write(PacketType::UpdateResponse);

    // Send new entities that don't exist on the client
    {
        auto currentState = GetCurrentWorldState();
        auto& clientState = _clientStateByClientId[clientId];

        WorldDiff diff(clientState.currentState, currentState);

        for(auto addedEntity : diff.addedEntities)
        {   
            auto net = _componentsByNetId[addedEntity];
            auto entity = net->owner;

            Log("Send entity id #%d (%s)\n", addedEntity, typeid(*entity).name());

            SpawnEntityMessage spawnMessage;
            spawnMessage.position = entity->Center();
            spawnMessage.netId = net->netId;
            spawnMessage.type = entity->type;
            spawnMessage.dimensions = entity->Dimensions();

            response.Write(MessageType::SpawnEntity);
            spawnMessage.ReadWrite(responseStream);
        }

        clientState.currentState = currentState;
    }

    // Send the current state of the world
    {
        response.Write(MessageType::EntitySnapshot);

        EntitySnapshotMessage responseMessage;
        responseMessage.lastServerSequence = client != nullptr ? client->lastServerSequenceNumber : 0;
        responseMessage.lastServerExecuted = client != nullptr ? client->lastServedExecuted : 0;
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

        responseMessage.ReadWrite(responseStream);
    }
}

WorldState ReplicationManager::GetCurrentWorldState()
{
    WorldState state;
    for(auto component : components)
    {
        state.entities.insert(component->netId);
    }

    return state;
}

void ReplicationManager::ProcessSpawnEntity(ReadWriteBitStream& stream)
{
    SpawnEntityMessage message;
    message.ReadWrite(stream);

    EntityDictionary properties
    {
        { "type", message.type },
        { "position", message.position },
        { "dimensions", message.dimensions }
    };

    auto entity = _scene->CreateEntity(properties);
    auto netComponent = entity->GetComponent<NetComponent>();

    if (netComponent != nullptr)
    {
        netComponent->netId = message.netId;

        _componentsByNetId[netComponent->netId] = netComponent;
        components.insert(netComponent);

        entity->SendEvent(SpawnedOnClientEvent());
    }
}

void ReplicationManager::ProcessEntitySnapshotMessage(ReadWriteBitStream& stream)
{
    EntitySnapshotMessage message;
    message.ReadWrite(stream);

    if(localPlayer.GetValueOrNull() == nullptr)
    {
        return;
    }

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
        else if (player != self || true)
        {
            if (lastCommandExecuted != nullptr)
            {
                PlayerSnapshot snapshot;
                snapshot.commandId = self->lastServedExecuted;
                snapshot.position = message.entities[i].position;
                snapshot.time = lastCommandExecuted->timeRecorded;
                player->AddSnapshot(snapshot);
            }
        }
        else
        {
            self->positionAtStartOfCommand = message.entities[i].position;
        }
    }
}