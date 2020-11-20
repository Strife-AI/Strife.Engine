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
            .Add(dimensions)
            .Add(ownerClientId);
    }

    uint16 netId;
    uint8 ownerClientId;
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
        stream
            .Add(keys)
            .Add(fixedUpdateCount)
            .Add(moveToTarget)
            .Add(target)
            .Add(netId)
            .Add(attackTarget)
            .Add(attackNetId);
    }

    uint8 keys;
    uint8 fixedUpdateCount;
    bool moveToTarget;
    bool attackTarget;
    uint32 attackNetId;
    Vector2 target;
    uint32 netId;
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

PlayerCommand* ClientState::GetCommandById(int id)
{
    for (auto& command : commands)
    {
        if (command.id == id)
        {
            return &command;
        }
    }

    return nullptr;
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

    if(localClientId == -1)
    {
        return;
    }
    else
    {
        // TODO: this isn't necessary?
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

    // Time to send new update to server with missing commands
    if (_sendUpdateTimer <= 0 || true)
    {
        _sendUpdateTimer += 1.0 / 30;

        auto& client = _clientStateByClientId[localClientId];

        // Garbage collect old commands that the server already has
        {
            while (!client.commands.IsEmpty() && client.commands.Peek().id < client.lastServedExecuted)
            {
                client.commands.Dequeue();
            }
        }

        ClientUpdateRequestMessage request;
        int commandCount = 0;

        for (auto& command : client.commands)
        {
            if (command.id > client.lastServerSequenceNumber)
            {
                if (commandCount == 0)
                {
                    request.firstCommandId = command.id;
                }

                request.commands[commandCount].keys = command.keys;
                request.commands[commandCount].fixedUpdateCount = command.fixedUpdateCount; // TODO: clamp
                request.commands[commandCount].target = command.target;
                request.commands[commandCount].moveToTarget = command.moveToTarget;
                request.commands[commandCount].netId = command.netId;
                request.commands[commandCount].moveToTarget = command.moveToTarget;
                request.commands[commandCount].attackNetId = command.attackNetId;

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
}

int BitsNeeded(int value)
{
    int bits = 0;
    while (value > 0)
    {
        ++bits;
        value >>= 1;
    }

    return bits;
}

struct VarGroup
{
    ISyncVar* vars[32];
    bool changed[32];
    int changedCount = 0;
    int varCount = 0;
};

void WriteVarGroup(VarGroup* group, uint32 lastClientSequenceId, SLNet::BitStream& out)
{
    for (int i = 0; i < group->varCount; ++i)
    {
        if (!group->vars[i]->IsBool())
        {
            bool changed = group->changed[i];
            out.Write(changed);

            if (changed)
            {
                group->vars[i]->WriteValueDeltaedFromSequence(lastClientSequenceId, out);
            }
        }
        else
        {
            // Writing a bit for whether a bool has changed just wastes a bit, so just write the bit
            group->vars[i]->WriteValueDeltaedFromSequence(lastClientSequenceId, out);
        }
    }
}

void ReadVarGroup(VarGroup* group, uint32 lastClientSequenceId, float time, SLNet::BitStream& stream)
{
    for (int i = 0; i < group->varCount; ++i)
    {
        if (!group->vars[i]->IsBool())
        {
            bool wasChanged;
            stream.Read(wasChanged);

            if (wasChanged)
            {
                group->vars[i]->ReadValueDeltaedFromSequence(lastClientSequenceId, time, stream);
            }
        }
        else
        {
            group->vars[i]->ReadValueDeltaedFromSequence(lastClientSequenceId, time, stream);
        }
    }
}

void PartitionVars(ISyncVar* head, VarGroup* frequent, VarGroup* infrequent)
{
    for (auto var = head; var != nullptr; var = var->next)
    {
        VarGroup* group = var->frequency == SyncVarUpdateFrequency::Frequent
            ? frequent
            : infrequent;

        group->vars[group->varCount++] = var;
    }
}

void CheckForChangedVars(VarGroup* group, uint32 lastClientSequenceId)
{
    for (int i = 0; i < group->varCount; ++i)
    {
        group->changed[i] = group->vars[i]->CurrentValueChangedFromSequence(lastClientSequenceId);
        group->changedCount += group->changed[i];
    }
}

void WriteVars(ISyncVar* head, uint32 lastClientSequenceId, SLNet::BitStream& out)
{
    VarGroup frequent;
    VarGroup infrequent;

    PartitionVars(head, &frequent, &infrequent);
    CheckForChangedVars(&frequent, lastClientSequenceId);
    CheckForChangedVars(&infrequent, lastClientSequenceId);

    if (frequent.changedCount == 0 && infrequent.changedCount == 0)
    {
        out.Write0();
    }
    else
    {
        out.Write1();

        WriteVarGroup(&frequent, lastClientSequenceId, out);

        if(infrequent.varCount == 0)
        {
            return;
        }
        else if(infrequent.varCount == 1)
        {
            WriteVarGroup(&infrequent, lastClientSequenceId, out);
        }
        else
        {
            bool anyInfrequentChanges = infrequent.changedCount > 0;
            out.Write(anyInfrequentChanges);

            if(anyInfrequentChanges)
            {
                WriteVarGroup(&infrequent, lastClientSequenceId, out);
            }
        }
    }
}

void ReadVars(ISyncVar* head, uint32 lastClientSequenceId, float time, SLNet::BitStream& stream)
{
    VarGroup frequent;
    VarGroup infrequent;

    PartitionVars(head, &frequent, &infrequent);

    bool anyChanged = stream.ReadBit();

    if (!anyChanged)
    {
        return;
    }
    else
    {
        ReadVarGroup(&frequent, lastClientSequenceId, time, stream);

        if (infrequent.varCount == 0)
        {
            return;
        }
        else if (infrequent.varCount == 1)
        {
            ReadVarGroup(&infrequent, lastClientSequenceId, time, stream);
        }
        else
        {
            bool anyInfrequentChanges = stream.ReadBit();

            if (anyInfrequentChanges)
            {
                ReadVarGroup(&infrequent, lastClientSequenceId, time, stream);
            }
        }
    }
}

void ReplicationManager::ProcessMessageFromClient(SLNet::BitStream& message, SLNet::BitStream& response, int clientId)
{
    auto& client = _clientStateByClientId[clientId];

    ClientUpdateRequestMessage request;
    ReadWriteBitStream readMessage(message, true);
    request.ReadWrite(readMessage);

    if (client.lastServerSequenceNumber + 1 <= request.firstCommandId)
    {
        for (int i = 0; i < request.commandCount; ++i)
        {
            unsigned int currentId = request.firstCommandId + i;
            if (currentId > client.lastServerSequenceNumber)
            {
                PlayerCommand newCommand;
                newCommand.id = currentId;
                newCommand.keys = request.commands[i].keys;
                newCommand.fixedUpdateCount = request.commands[i].fixedUpdateCount;
                newCommand.moveToTarget = request.commands[i].moveToTarget;
                newCommand.target = request.commands[i].target;
                newCommand.netId = request.commands[i].netId;
                newCommand.attackTarget = request.commands[i].attackTarget;
                newCommand.attackNetId = request.commands[i].attackNetId;
                client.lastServerSequenceNumber = currentId;

                if (client.commands.IsFull())
                {
                    client.commands.Dequeue();
                }

                client.commands.Enqueue(newCommand);
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
            spawnMessage.ownerClientId = net->ownerClientId;

            response.Write(MessageType::SpawnEntity);
            spawnMessage.ReadWrite(responseStream);
        }

        clientState.currentState = currentState;
    }

    // Send the current state of the world
    {
        response.Write(MessageType::EntitySnapshot);

        EntitySnapshotMessage responseMessage;
        responseMessage.lastServerSequence = client.lastServerSequenceNumber;
        responseMessage.lastServerExecuted = client.lastServedExecuted;
        responseMessage.totalEntities = components.size();

        responseMessage.ReadWrite(responseStream);

        for (auto c : components)
        {
            uint8 netId = c->netId;
            response.WriteBits(&netId, 8);
            WriteVars(c->owner->syncVarHead, 0, response);
        }
    }
}

void ReplicationManager::AddPlayerCommand(const PlayerCommand& command)
{
    if(localClientId == -1)
    {
        return;
    }

    auto copy = command;
    auto& client = _clientStateByClientId[localClientId];

    copy.id = ++client.nextCommandSequenceNumber;
    client.commands.Enqueue(copy);
}

WorldState ReplicationManager::GetCurrentWorldState()
{
    WorldState state;
    for (auto component : components)
    {
        state.entities.insert(component->netId);
    }

    return state;
}

void ReplicationManager::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<EndOfUpdateEvent>())
    {
        if (_isServer)
        {
            for (auto c : components)
            {
                for (auto var = c->owner->syncVarHead; var != nullptr; var = var->next)
                {
                    var->AddCurrentValueToSnapshots(_currentSnapshotId, scene->relativeTime);
                }
            }
        }
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
        { "dimensions", message.dimensions },
        { "net", true }
    };

    auto entity = _scene->CreateEntity(properties);
    auto netComponent = entity->GetComponent<NetComponent>();

    if (netComponent != nullptr)
    {
        netComponent->netId = message.netId;
        netComponent->ownerClientId = message.ownerClientId;

        _componentsByNetId[netComponent->netId] = netComponent;
        components.insert(netComponent);

        entity->SendEvent(SpawnedOnClientEvent());
    }
}

void ReplicationManager::ProcessEntitySnapshotMessage(ReadWriteBitStream& stream)
{
    EntitySnapshotMessage message;
    message.ReadWrite(stream);

    if(localClientId == -1)
    {
        return;
    }

    auto& client = _clientStateByClientId[localClientId];

    client.lastServerSequenceNumber = message.lastServerSequence;
    client.lastServedExecuted = message.lastServerExecuted;

    for (int i = 0; i < message.totalEntities; ++i)
    {
        uint8 netId;
        stream.stream.ReadBits(&netId, 8);

        auto player = _componentsByNetId.count(netId) != 0
            ? _componentsByNetId[netId]
            : nullptr;

        if(!player)
        {
            FatalError("Missing entity %d\n", (int)netId);
        }

        auto cmd = client.GetCommandById(client.lastServedExecuted);

        float time = cmd == nullptr
            ? -1
            : cmd->timeRecorded;

        ReadVars(player->owner->syncVarHead, 0, time, stream.stream);
    }
}
