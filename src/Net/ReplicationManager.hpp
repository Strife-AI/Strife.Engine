#pragma once
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "Memory/CircularQueue.hpp"


#include "Components/NetComponent.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

struct ClientGame;
struct NetworkInterface;
class NetworkManager;

namespace SLNet {
    class BitStream;
}

enum class MessageType : uint8;
struct ReadWriteBitStream;
class WorldState;

struct WorldDiff
{
    void Merge(const WorldDiff& rhs);

    std::vector<int> addedEntities;
    std::vector<int> destroyedEntities;
};

struct WorldState
{
    static void Diff(const WorldState& before, const WorldState& after, WorldDiff& outDiff);

    std::vector<int> entities;
    unsigned int snapshotId;
    WorldDiff diffFromLastSnapshot;
};

DEFINE_EVENT(PlayerConnectedEvent)
{
    PlayerConnectedEvent(int id_, std::optional<Vector2> = std::nullopt)
        : id(id_)
    {

    }

    int id;
    std::optional<Vector2> position;
};

DEFINE_EVENT(PlayerInfoUpdatedEvent)
{
    PlayerInfoUpdatedEvent(int clientId)
            : clientId(clientId)
    {

    }

    int clientId;
};

DEFINE_EVENT(JoinedServerEvent)
{
    JoinedServerEvent(int selfId_)
        : selfId(selfId_)
    {

    }

    int selfId;
};

struct ClientState
{
    PlayerCommand* GetCommandById(int id);

    unsigned int lastServerSequenceNumber = 0;
    unsigned int lastReceivedSnapshotId = 0;
    unsigned int nextCommandSequenceNumber = 0;
    unsigned int lastServerExecuted = 0;
    int wasted = 0;

    unsigned int clientClock = 0;

    CircularQueue<PlayerCommand, 128> commands;
    std::string clientName;
};

class ReplicationManager : public ISceneService
{
public:
    ReplicationManager(Scene* scene, bool isServer);

    void AddNetComponent(NetComponent* component)
    {
        if (_isServer)
        {
            component->netId = _nextNetId++;
            _componentsByNetId[component->netId] = component;
            components.insert(component);
        }
    }

    void RemoveNetComponent(NetComponent* component)
    {
        if(!_isServer && !component->isMarkedForDestructionOnClient)
        {
            // FIXME: this doesn't work when destroying the scene
            //FatalError("Tried to remove net component from %s on client\n", typeid(*component->owner).name());
        }

        _componentsByNetId.erase(component->netId);
        components.erase(component);
    }

    NetComponent* GetNetComponentById(int id) const
    {
        auto it = _componentsByNetId.find(id);
        return it != _componentsByNetId.end()
            ? it->second
            : nullptr;
    }

    auto& GetClients() { return _clientStateByClientId; }
    ClientState& GetClient(int clientId) { return _clientStateByClientId[clientId]; }

    void Client_ReceiveUpdateResponse(SLNet::BitStream& stream);
    void Client_SendUpdateRequest(float deltaTime, ClientGame* game);
    void Client_AddPlayerCommand(const PlayerCommand& command);

    void Server_ProcessUpdateRequest(SLNet::BitStream& message, int clientId);
    bool Server_SendWorldUpdate(int clientId, SLNet::BitStream &response);

    void Server_ClientDisconnected(int clientId);

    void TakeSnapshot();

    WorldState GetCurrentWorldState();

    uint32 GetCurrentSnapshotId() const { return _currentSnapshotId; }

    std::unordered_set<NetComponent*> components;
    int localClientId = -1;
	PlayerCommandHandler playerCommandHandler;

private:
    void ReceiveEvent(const IEntityEvent& ev) override;

    void ProcessSpawnEntity(class SpawnEntityMessage& message);
    void ProcessDestroyEntity(class DestroyEntityMessage& message, float destroyTime);
    void ProcessEntitySnapshotMessage(ReadWriteBitStream& stream, uint32 snapshotFromId);

    WorldState* GetWorldSnapshot(uint32 snapshotId);

    std::map<int, NetComponent*> _componentsByNetId;
    std::unordered_map<int, ClientState> _clientStateByClientId;

    int _nextNetId = 0;
    bool _isServer;
    Scene* _scene;
    float _sendUpdateTimer = 0;

    uint32 _currentSnapshotId = 0;
    CircularQueue<WorldState, 32> _worldSnapshots;
};