#include "ReplicationManager.hpp"

#include <slikenet/BitStream.h>


#include "NetworkManager.hpp"
#include "Scene/Scene.hpp"

enum class MessageType : uint8
{
    SpawnEntity,
    RemoveEntity,
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

    uint8 netId;
    uint8 ownerClientId;
    StringId type;
    Vector2 position;
    Vector2 dimensions;
};

struct DestroyEntityMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream.Add(netId);
    }

    uint8 netId;
};

struct UpdateResponseMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream
            .Add(snapshotFrom)
            .Add(snapshotTo);
    }

    uint32 snapshotFrom;
    uint32 snapshotTo;
};

struct EntitySnapshotMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream
            .Add(lastServerSequence)
            .Add(lastServerExecuted);
    }

    int lastServerSequence;
    int lastServerExecuted;
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
        stream.Add(lastReceivedSnapshotId);
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
    uint32 lastReceivedSnapshotId;
};

WorldDiff::WorldDiff(const WorldState& before, const WorldState& after)
{
    int beforeIndex = 0;
    int afterIndex = 0;

    while (beforeIndex < before.entities.size() && afterIndex < after.entities.size())
    {
        if (before.entities[beforeIndex] < after.entities[afterIndex])
        {
            destroyedEntities.push_back(before.entities[beforeIndex++]);
        }
        else if(before.entities[beforeIndex] > after.entities[afterIndex])
        {
            addedEntities.push_back(after.entities[afterIndex++]);
        }
        else
        {
            ++beforeIndex;
            ++afterIndex;
        }
    }

    while(beforeIndex < before.entities.size())
    {
        destroyedEntities.push_back(before.entities[beforeIndex++]);
    }

    while(afterIndex < after.entities.size())
    {
        addedEntities.push_back(after.entities[afterIndex++]);
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

void ReplicationManager::Client_ReceiveUpdateResponse(SLNet::BitStream& stream)
{
    ReadWriteBitStream rw(stream, true);

    UpdateResponseMessage responseMessage;
    responseMessage.ReadWrite(rw);

    if(localClientId == -1)
    {
        return;
    }

    auto& client = _clientStateByClientId[localClientId];
    if(responseMessage.snapshotTo <= client.lastReceivedSnapshotId)
    {
        return;
    }

    client.lastReceivedSnapshotId = responseMessage.snapshotTo;

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
            ProcessEntitySnapshotMessage(rw, responseMessage.snapshotFrom);
            break;

        case MessageType::RemoveEntity:
            ProcessDestroyEntity(rw);
            break;

        default:
            break;
        }
    }
}

void ReplicationManager::Client_SendUpdateRequest(float deltaTime, NetworkManager* networkManager)
{
    _sendUpdateTimer -= _scene->deltaTime;

    if(localClientId == -1)
    {
        return;
    }

    // Time to send new update to server with missing commands
    if (_sendUpdateTimer <= 0)
    {
        _sendUpdateTimer += 1.0 / 30;

        auto& client = _clientStateByClientId[localClientId];

        // Garbage collect old commands that the server already has
        {
            while (!client.commands.IsEmpty() && client.commands.Peek().id < client.lastServerExecuted)
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
        request.lastReceivedSnapshotId = client.lastReceivedSnapshotId;

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

void WriteVarGroup(VarGroup* group, uint32 fromSnapshotId, uint32 toSnapshotId, SLNet::BitStream& out)
{
    for (int i = 0; i < group->varCount; ++i)
    {
        if (!group->vars[i]->IsBool())
        {
            bool changed = group->changed[i];
            out.Write(changed);

            if (changed)
            {
                group->vars[i]->WriteValueDeltaedFromSnapshot(fromSnapshotId, toSnapshotId, out);
            }
        }
        else
        {
            // Writing a bit for whether a bool has changed just wastes a bit, so just write the bit
            group->vars[i]->WriteValueDeltaedFromSnapshot(fromSnapshotId, toSnapshotId, out);
        }
    }
}

void ReadVarGroup(VarGroup* group, uint32 fromSnapshotId, uint32 toSnapshotId, float time, SLNet::BitStream& stream)
{
    for (int i = 0; i < group->varCount; ++i)
    {
        if (!group->vars[i]->IsBool())
        {
            bool wasChanged;
            stream.Read(wasChanged);

            if (wasChanged)
            {
                group->vars[i]->ReadValueDeltaedFromSequence(fromSnapshotId, toSnapshotId,  time, stream);
            }
        }
        else
        {
            group->vars[i]->ReadValueDeltaedFromSequence(fromSnapshotId, toSnapshotId, time, stream);
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

void WriteVars(ISyncVar* head, uint32 fromSnapshotId, uint32 toSnapshotId, SLNet::BitStream& out)
{
    VarGroup frequent;
    VarGroup infrequent;

    PartitionVars(head, &frequent, &infrequent);
    CheckForChangedVars(&frequent, fromSnapshotId);
    CheckForChangedVars(&infrequent, fromSnapshotId);

    if (frequent.changedCount == 0 && infrequent.changedCount == 0)
    {
        out.Write0();
    }
    else
    {
        out.Write1();

        WriteVarGroup(&frequent, fromSnapshotId, toSnapshotId, out);

        if(infrequent.varCount == 0)
        {
            return;
        }
        else if(infrequent.varCount == 1)
        {
            WriteVarGroup(&infrequent, fromSnapshotId, toSnapshotId, out);
        }
        else
        {
            bool anyInfrequentChanges = infrequent.changedCount > 0;
            out.Write(anyInfrequentChanges);

            if(anyInfrequentChanges)
            {
                WriteVarGroup(&infrequent, fromSnapshotId, toSnapshotId, out);
            }
        }
    }
}

void ReadVars(ISyncVar* head, uint32 fromSnapshotId, uint32 toSnapshotId, float time, SLNet::BitStream& stream)
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
        ReadVarGroup(&frequent, fromSnapshotId, toSnapshotId, time, stream);

        if (infrequent.varCount == 0)
        {
            return;
        }
        else if (infrequent.varCount == 1)
        {
            ReadVarGroup(&infrequent, fromSnapshotId, toSnapshotId, time, stream);
        }
        else
        {
            bool anyInfrequentChanges = stream.ReadBit();

            if (anyInfrequentChanges)
            {
                ReadVarGroup(&infrequent, fromSnapshotId, toSnapshotId, time, stream);
            }
        }
    }
}

static WorldState g_emptyWorldState;

void ReplicationManager::Server_ProcessUpdateRequest(SLNet::BitStream& message, SLNet::BitStream& response, int clientId)
{
    auto& client = _clientStateByClientId[clientId];

    // Read the commands that the client sent to us and add them to the server's list of commands for that client
    {
        ClientUpdateRequestMessage request;
        ReadWriteBitStream readMessage(message, true);
        request.ReadWrite(readMessage);

        client.lastReceivedSnapshotId = Max(client.lastReceivedSnapshotId, request.lastReceivedSnapshotId);

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
    }

    ReadWriteBitStream responseStream(response, false);

    response.Write(PacketType::UpdateResponse);

    if(client.lastReceivedSnapshotId == _currentSnapshotId)
    {
        return;
    }

    UpdateResponseMessage responseMessage;
    responseMessage.snapshotFrom = client.lastReceivedSnapshotId;
    responseMessage.snapshotTo = _currentSnapshotId;
    responseMessage.ReadWrite(responseStream);

    {
        auto currentState = GetWorldSnapshot(_currentSnapshotId);
        if (currentState == nullptr) FatalError("Missing current snapshot");

        auto& clientState = _clientStateByClientId[clientId];

        auto lastClientState = GetWorldSnapshot(clientState.lastReceivedSnapshotId);

        if (lastClientState == nullptr)
        {
            Log("Have to send from %d -> %d\n", clientState.lastReceivedSnapshotId, _currentSnapshotId);
            lastClientState = &g_emptyWorldState;
        }

        WorldDiff diff(*lastClientState, *currentState);

        // Send destroy messages for entities that the client doesn't know were destroyed
        for (auto destroyedEntity : diff.destroyedEntities)
        {
            DestroyEntityMessage destroyMessage;
            destroyMessage.netId = destroyedEntity;

            response.Write(MessageType::RemoveEntity);
            destroyMessage.ReadWrite(responseStream);
        }

        // Send new entities that don't exist on the client
        for (auto addedEntity : diff.addedEntities)
        {   
            auto net = _componentsByNetId[addedEntity];
            auto entity = net->owner;

            SpawnEntityMessage spawnMessage;
            spawnMessage.position = entity->Center();
            spawnMessage.netId = net->netId;
            spawnMessage.type = entity->type;
            spawnMessage.dimensions = entity->Dimensions();
            spawnMessage.ownerClientId = net->ownerClientId;

            response.Write(MessageType::SpawnEntity);
            spawnMessage.ReadWrite(responseStream);
        }
    }

    // Send the current state of the world
    {
        response.Write(MessageType::EntitySnapshot);

        EntitySnapshotMessage responseMessage;
        responseMessage.lastServerSequence = client.lastServerSequenceNumber;
        responseMessage.lastServerExecuted = client.lastServerExecuted;

        responseMessage.ReadWrite(responseStream);

        for (auto& c : _componentsByNetId)
        {
            auto fromSnapshotId = client.lastReceivedSnapshotId;
            auto toSnapshotId = _currentSnapshotId;

            WriteVars(c.second->owner->syncVarHead, fromSnapshotId, toSnapshotId, response);
        }
    }
}

void ReplicationManager::Client_AddPlayerCommand(const PlayerCommand& command)
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
    state.snapshotId = _currentSnapshotId;

    for (auto& component : _componentsByNetId)
    {
        if (!component.second->owner->isDestroyed)
        {
            state.entities.push_back(component.first);
        }
    }

    return state;
}

void ReplicationManager::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<EndOfUpdateEvent>())
    {
        ++_currentSnapshotId;
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

        if(_worldSnapshots.IsFull())
        {
            _worldSnapshots.Dequeue();
        }

        _worldSnapshots.Enqueue(GetCurrentWorldState());
    }
}

void ReplicationManager::ProcessSpawnEntity(ReadWriteBitStream& stream)
{
    SpawnEntityMessage message;
    message.ReadWrite(stream);

    if(_componentsByNetId.count(message.netId) != 0)
    {
        // Duplicate entity
        return;
    }

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

void ReplicationManager::ProcessDestroyEntity(ReadWriteBitStream& stream)
{
    DestroyEntityMessage message;
    message.ReadWrite(stream);

    auto component = _componentsByNetId.find(message.netId);

    if (component != _componentsByNetId.end())
    {
        auto net = component->second;
        components.erase(net);
        _componentsByNetId.erase(component->second->netId);
        net->owner->Destroy();
    }
}

void ReplicationManager::ProcessEntitySnapshotMessage(ReadWriteBitStream& stream, uint32 snapshotFromId)
{
    EntitySnapshotMessage message;
    message.ReadWrite(stream);

    if(localClientId == -1)
    {
        return;
    }

    auto& client = _clientStateByClientId[localClientId];

    client.lastServerSequenceNumber = message.lastServerSequence;
    client.lastServerExecuted = message.lastServerExecuted;

    for (auto& c : _componentsByNetId)
    {
        auto cmd = client.GetCommandById(client.lastServerExecuted);

        float time = cmd == nullptr
            ? -1
            : cmd->timeRecorded;

        ReadVars(c.second->owner->syncVarHead, snapshotFromId, client.lastReceivedSnapshotId, time, stream.stream);
    }
}

WorldState* ReplicationManager::GetWorldSnapshot(uint32 snapshotId)
{
    for(auto& worldSnapshot : _worldSnapshots)
    {
        if(worldSnapshot.snapshotId == snapshotId)
        {
            return &worldSnapshot;
        }
    }

    return nullptr;
}
