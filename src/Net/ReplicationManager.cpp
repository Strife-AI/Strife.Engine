#include "ReplicationManager.hpp"

#include <slikenet/BitStream.h>

#include "Net/ServerGame.hpp"
#include "Scene/Scene.hpp"
#include "Engine.hpp"


#ifdef _WIN32
namespace SLNet
{
    const RakNetGUID UNASSIGNED_RAKNET_GUID((uint64_t)-1);
    const SystemAddress UNASSIGNED_SYSTEM_ADDRESS;
}
#endif

//#define NET_DEBUG

#ifdef NET_DEBUG
#define NetLog printf
#else

void NetLog(...)
{
}

#endif


enum class MessageType : uint8
{
    EntitySnapshot
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

    int netId;
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

    int netId;
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
            .Add(sentTime)
            .Add(lastServerSequence)
            .Add(lastServerExecuted)
            .Add(totalEntities);
    }

    float sentTime;
    int lastServerSequence;
    int lastServerExecuted;
    int totalEntities;
};

struct ClientUpdateRequestMessage
{
    void ReadWrite(ReadWriteBitStream& stream)
    {
        stream.Add(lastReceivedSnapshotId);
        stream.Add(firstCommandId);
    }

    uint32 firstCommandId;
    uint32 lastReceivedSnapshotId;
};

void WorldDiff::Merge(const WorldDiff& rhs)
{
    addedEntities.insert(addedEntities.end(), rhs.addedEntities.begin(), rhs.addedEntities.end());
    destroyedEntities.insert(destroyedEntities.end(), rhs.destroyedEntities.begin(), rhs.destroyedEntities.end());
}

PlayerCommand* ClientState::GetCommandById(int id)
{
    for (auto& command : commands)
    {
        if (command->id == id)
        {
            return command;
        }
    }

    return nullptr;
}

void ReplicationManager::Client_ReceiveUpdateResponse(SLNet::BitStream& stream)
{
    ReadWriteBitStream rw(stream, true);

    UpdateResponseMessage responseMessage;
    responseMessage.ReadWrite(rw);

    if (localClientId == -1)
    {
        return;
    }

    auto& client = _clientStateByClientId[localClientId];
    if (responseMessage.snapshotTo <= client.lastReceivedSnapshotId)
    {
        NetLog("Receive out of order message\n");
        return;
    }

    client.lastReceivedSnapshotId = responseMessage.snapshotTo;

    NetLog("=========================Receive Update Response=========================\n");

    while (stream.GetNumberOfUnreadBits() >= 8)
    {
        uint8 messageType;
        stream.Read(messageType);

        switch ((MessageType)messageType)
        {
        case MessageType::EntitySnapshot:
            NetLog("    Receive entity snapshot\n");
            ProcessEntitySnapshotMessage(rw, responseMessage.snapshotFrom);
            break;

        default:
            NetLog("Unknown message type: %d\n", messageType);
            break;
        }
    }
}

void ReplicationManager::Client_SendUpdateRequest(float deltaTime, ClientGame* game)
{
    _sendUpdateTimer -= _scene->deltaTime;

    if (localClientId == -1)
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
            while (!client.commands.IsEmpty() && client.commands.Peek()->id < client.lastServerExecutedCommandId)
            {
                auto oldCommand = client.commands.Dequeue();
                playerCommandHandler.FreeCommand(*oldCommand);
            }
        }

        const int maxPacketSize = 1300;

        ClientUpdateRequestMessage request;
        request.lastReceivedSnapshotId = client.lastReceivedSnapshotId;

        SLNet::BitStream message;
        message.Write(PacketType::UpdateRequest);
        ReadWriteBitStream stream(message, false);

        PlayerCommandHandler::ScheduledCommand* commands[decltype(playerCommandHandler.localCommands)::Capacity()];
        int totalCommands = 0;

        for (auto& command : playerCommandHandler.localCommands)
        {
            if (command.commandId > client.lastServerReceivedCommandId)
            {
                commands[totalCommands++] = &command;
            }
        }

        if (totalCommands == 0)
        {
            request.firstCommandId = 0;
        }
        else
        {
            request.firstCommandId = commands[0]->commandId;
        }

        request.ReadWrite(stream);

        for (int i = 0; i < totalCommands; ++i)
        {
            commands[i]->stream.ResetReadPointer();
            bool hasRoomForCommand =
                message.GetNumberOfBitsUsed() + commands[i]->stream.GetNumberOfUnreadBits() <= maxPacketSize * 8;
            if (hasRoomForCommand)
            {
                message.Write(commands[i]->stream);
            }
            else
            {
                break;
            }
        }

        game->networkInterface.SendReliable(game->serverAddress, message);
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

            NetLog("   - Changed: %d (%s)\n", wasChanged, typeid(*group->vars[i]).name());

            if (wasChanged)
            {
                group->vars[i]->ReadValueDeltaedFromSequence(fromSnapshotId, toSnapshotId, time, stream);
            }
        }
        else
        {
            NetLog("    - Read bool\n");
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

        if (infrequent.varCount == 0)
        {
            return;
        }
        else if (infrequent.varCount == 1)
        {
            WriteVarGroup(&infrequent, fromSnapshotId, toSnapshotId, out);
        }
        else
        {
            bool anyInfrequentChanges = infrequent.changedCount > 0;
            out.Write(anyInfrequentChanges);

            if (anyInfrequentChanges)
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

    NetLog("====Begin read vars====\n");
    NetLog("Any changed: %d\n", anyChanged);

    if (!anyChanged)
    {
        return;
    }
    else
    {
        NetLog("Reading frequent\n");
        ReadVarGroup(&frequent, fromSnapshotId, toSnapshotId, time, stream);

        if (infrequent.varCount == 0)
        {
            NetLog("No infrequent vars\n");
        }
        else if (infrequent.varCount == 1)
        {
            NetLog("One infrequent var\n");
            ReadVarGroup(&infrequent, fromSnapshotId, toSnapshotId, time, stream);
        }
        else
        {
            NetLog("Multiple infrequent vars\n");
            bool anyInfrequentChanges = stream.ReadBit();

            NetLog("Any infrequent changed: %d\n", anyInfrequentChanges);

            if (anyInfrequentChanges)
            {
                ReadVarGroup(&infrequent, fromSnapshotId, toSnapshotId, time, stream);
            }
        }
    }
}

static WorldState g_emptyWorldState;

void ReplicationManager::Server_ProcessUpdateRequest(SLNet::BitStream& message, int clientId)
{
    auto& client = _clientStateByClientId[clientId];

    // Read the commands that the client sent to us and add them to the server's list of commands for that client
    {
        ClientUpdateRequestMessage request;
        ReadWriteBitStream readMessage(message, true);
        request.ReadWrite(readMessage);

        client.lastReceivedSnapshotId = Max(client.lastReceivedSnapshotId, request.lastReceivedSnapshotId);

        if (client.lastServerReceivedCommandId + 1 <= request.firstCommandId)
        {
            unsigned int currentCommandId = request.firstCommandId;

            while (message.GetNumberOfUnreadBits() >= 8)
            {
                auto command = playerCommandHandler.DeserializeCommand(readMessage);
                command->id = currentCommandId++;

                // Only add commands we haven't already received
                if (command->id > client.lastServerReceivedCommandId)
                {
                    if (client.commands.IsFull())
                    {
                        auto oldCommand = client.commands.Dequeue();
                        playerCommandHandler.FreeCommand(*oldCommand);
                    }

                    client.commands.Enqueue(command);
                    client.lastServerReceivedCommandId = command->id;
                }
                else
                {
                    // Duplicate
                    playerCommandHandler.FreeCommand(command);
                }
            }
        }
    }
}

bool ReplicationManager::Server_SendWorldUpdate(int clientId, SLNet::BitStream& response)
{
    auto& client = _clientStateByClientId[clientId];
    ReadWriteBitStream responseStream(response, false);

    response.Write(PacketType::UpdateResponse);

    if (client.lastReceivedSnapshotId == _currentSnapshotId)
    {
        return false;
    }

    UpdateResponseMessage responseMessage;
    responseMessage.snapshotFrom = client.lastReceivedSnapshotId;
    responseMessage.snapshotTo = _currentSnapshotId;
    responseMessage.ReadWrite(responseStream);

    response.Write(MessageType::EntitySnapshot);

    auto currentState = GetWorldSnapshot(_currentSnapshotId);
    if (currentState == nullptr) FatalError("Missing current snapshot");

    {
        auto& clientState = _clientStateByClientId[clientId];

        auto lastClientState = GetWorldSnapshot(clientState.lastReceivedSnapshotId);

        WorldDiff diff;
        if (lastClientState == nullptr)
        {
            NetLog("Have to send from %d -> %d\n", clientState.lastReceivedSnapshotId, _currentSnapshotId);
            lastClientState = &g_emptyWorldState;

            diff.addedEntities = currentState->entities;
        }
        else
        {
            bool includeDiff = false;
            for (auto& snapshot : _worldSnapshots)
            {
                if (snapshot.snapshotId == clientState.lastReceivedSnapshotId)
                {
                    includeDiff = true;
                }

                if (includeDiff)
                {
                    diff.Merge(snapshot.diffFromLastSnapshot);
                }

                if (snapshot.snapshotId == _currentSnapshotId)
                {
                    break;
                }
            }
        }

        NetLog("Entities believed to be on client: %d\n", (int)lastClientState->entities.size());

        NetLog("On client: ");
        for (int i = 0; i < lastClientState->entities.size(); ++i)
        {
            NetLog("%d \n", lastClientState->entities[i]);
        }

        NetLog("\n");

        NetLog("On server: ");
        for (int i = 0; i < currentState->entities.size(); ++i)
        {
            NetLog("%d \n", currentState->entities[i]);
        }

        NetLog("\n");

        NetLog("Entities added: %d\n", (int)diff.addedEntities.size());
        NetLog("Entities destroyed: %d\n", (int)diff.destroyedEntities.size());

        // Send new entities that don't exist on the client
        for (auto addedEntity : diff.addedEntities)
        {
            if (_componentsByNetId.count(addedEntity) == 0 || _componentsByNetId[addedEntity]->owner->isDestroyed)
            {
                // Never need to send created events if the entity was destroyed in this packet
                continue;
            }

            auto net = _componentsByNetId[addedEntity];

            auto entity = net->owner;

            if (entity->isDestroyed)
            {
                FatalError("Entity was destroyed");
            }

            SpawnEntityMessage spawnMessage;
            spawnMessage.position = entity->Center();
            spawnMessage.netId = net->netId;
            spawnMessage.type = entity->type;
            spawnMessage.dimensions = entity->Dimensions();
            spawnMessage.ownerClientId = net->ownerClientId;

            response.Write(true);
            spawnMessage.ReadWrite(responseStream);
        }

        response.Write(false);

        // Send message
        EntitySnapshotMessage responseMessage;
        responseMessage.lastServerSequence = client.lastServerReceivedCommandId;
        responseMessage.lastServerExecuted = client.lastServerExecutedCommandId;
        responseMessage.sentTime = scene->absoluteTime;
        responseMessage.totalEntities = currentState->entities.size();

        responseMessage.ReadWrite(responseStream);

        // Send destroy messages for entities that the client doesn't know were destroyed
        for (auto destroyedEntity : diff.destroyedEntities)
        {
            DestroyEntityMessage destroyMessage;
            destroyMessage.netId = destroyedEntity;

            response.Write(true);
            destroyMessage.ReadWrite(responseStream);
        }

        response.Write(false);
    }

    // Send the current state of the world
    {
        NetLog("Total entities on server: %d\n", (int)_componentsByNetId.size());

        for (auto& c : _componentsByNetId)
        {
            auto fromSnapshotId = client.lastReceivedSnapshotId;
            auto toSnapshotId = _currentSnapshotId;

            WriteVars(c.second->owner->syncVarHead, fromSnapshotId, toSnapshotId, response);
        }
    }

    return true;
}

WorldState ReplicationManager::GetCurrentWorldState()
{
    WorldState state;
    state.snapshotId = _currentSnapshotId;

    for (auto& component : _componentsByNetId)
    {
        if (!component.second->owner->isDestroyed)
            state.entities.push_back(component.first);
    }

    return state;
}

void ReplicationManager::ReceiveEvent(const IEntityEvent& ev)
{

}

void ReplicationManager::ProcessSpawnEntity(SpawnEntityMessage& message)
{
    if (_componentsByNetId.count(message.netId) != 0)
    {
        NetLog("Spawn duplicate entity: %d\n", message.netId);
        // Duplicate entity
        return;
    }

    NetLog("Spawn entity with netId %d\n", message.netId);

    EntitySerializer serializer;
    auto entity = _scene->CreateEntity(StringId(message.type), serializer);

    auto netComponent = entity->GetComponent<NetComponent>();

    if (netComponent != nullptr)
    {
        netComponent->netId = message.netId;
        netComponent->ownerClientId = message.ownerClientId;

        _componentsByNetId[netComponent->netId] = netComponent;
        components.insert(netComponent);

        entity->SendEvent(SpawnedOnClientEvent());
    }
    else
    {
        Log("Tried to spawn entity that does not have net component\n");
    }
}

void ReplicationManager::ProcessDestroyEntity(DestroyEntityMessage& message, float destroyTime)
{
    auto component = _componentsByNetId.find(message.netId);

    if (component != _componentsByNetId.end())
    {
        auto net = component->second;
        net->isMarkedForDestructionOnClient = true;
        net->destroyTime = destroyTime;
        net->markedForDestruction = true;
        NetLog("Destroyed entity %d\n", net->netId);
    }
    else
    {
        NetLog("Asked to destroy entity that didn't exist: %d\n", message.netId);
    }
}

void ReplicationManager::ProcessEntitySnapshotMessage(ReadWriteBitStream& stream, uint32 snapshotFromId)
{
    // Read new entities
    SpawnEntityMessage spawnEntityMessage;
    do
    {
        bool hasNewEntity = stream.stream.ReadBit();
        if (!hasNewEntity) break;

        spawnEntityMessage.ReadWrite(stream);
        ProcessSpawnEntity(spawnEntityMessage);
    } while (true);

    EntitySnapshotMessage message;
    message.ReadWrite(stream);
    float time = message.sentTime - scene->GetEngine()->GetClientGame()->GetServerClockOffset();

    // Read deleted entities
    DestroyEntityMessage destroyEntityMessage;
    do
    {
        bool hasDeletedEntity = stream.stream.ReadBit();
        if (!hasDeletedEntity) break;

        destroyEntityMessage.ReadWrite(stream);
        ProcessDestroyEntity(destroyEntityMessage, time);
    } while (true);

//    if(message.totalEntities != components.size())
//    {
//        fflush(stdout);
//        FatalError("Client and server disagree on number of entities (server: %d, client: %d)\n", message.totalEntities, (int)components.size());
//    }

    auto& client = _clientStateByClientId[localClientId];

    client.lastServerReceivedCommandId = message.lastServerSequence;
    client.lastServerExecutedCommandId = message.lastServerExecuted;

    NetLog("Total client-side entities: %d\n", (int)_componentsByNetId.size());

    for (auto& c : _componentsByNetId)
    {
        if (c.second->markedForDestruction)
        {
            continue;
        }

        ReadVars(c.second->owner->syncVarHead, snapshotFromId, client.lastReceivedSnapshotId, time, stream.stream);
    }
}

WorldState* ReplicationManager::GetWorldSnapshot(uint32 snapshotId)
{
    for (auto& worldSnapshot : _worldSnapshots)
    {
        if (worldSnapshot.snapshotId == snapshotId)
        {
            return &worldSnapshot;
        }
    }

    return nullptr;
}

void ReplicationManager::Server_ClientDisconnected(int clientId)
{
    for (auto c : components)
    {
        if (c->ownerClientId == clientId)
        {
            c->owner->Destroy();
        }

        _clientStateByClientId.erase(clientId);
    }
}

void ReplicationManager::TakeSnapshot()
{
    if (scene->deltaTime == 0)
    {
        return;
    }

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

    if (_worldSnapshots.IsFull())
    {
        _worldSnapshots.Dequeue();
    }

    auto state = GetCurrentWorldState();
    WorldState::Diff(_worldSnapshots.Last(), state, state.diffFromLastSnapshot);

    _worldSnapshots.Enqueue(std::move(state));
}

ReplicationManager::ReplicationManager(Scene* scene, bool isServer)
    : _isServer(isServer),
      _scene(scene),
      playerCommandHandler(scene->GetEngine()->GetDefaultBlockAllocator())
{
    _worldSnapshots.Enqueue(g_emptyWorldState);
}

void WorldState::Diff(const WorldState& before, const WorldState& after, WorldDiff& outDiff)
{
    int beforeIndex = 0;
    int afterIndex = 0;

    while (beforeIndex < before.entities.size() && afterIndex < after.entities.size())
    {
        if (before.entities[beforeIndex] < after.entities[afterIndex])
        {
            outDiff.destroyedEntities.push_back(before.entities[beforeIndex++]);
        }
        else if (before.entities[beforeIndex] > after.entities[afterIndex])
        {
            outDiff.addedEntities.push_back(after.entities[afterIndex++]);
        }
        else
        {
            ++beforeIndex;
            ++afterIndex;
        }
    }

    while (beforeIndex < before.entities.size())
    {
        outDiff.destroyedEntities.push_back(before.entities[beforeIndex++]);
    }

    while (afterIndex < after.entities.size())
    {
        outDiff.addedEntities.push_back(after.entities[afterIndex++]);
    }
}
