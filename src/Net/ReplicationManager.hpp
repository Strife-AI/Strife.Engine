#pragma once
#include <unordered_map>
#include <unordered_set>
#include "Memory/CircularQueue.hpp"


#include "Components/NetComponent.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

class NetworkManager;

namespace SLNet {
    class BitStream;
}

enum class MessageType : uint8;
struct ReadWriteBitStream;

struct WorldState
{
    std::vector<int> entities;
    unsigned int snapshotId;
};

struct WorldDiff
{
    WorldDiff(const WorldState& before, const WorldState& after);

    std::vector<int> addedEntities;
    std::vector<int> destroyedEntities;
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
};

class ReplicationManager : public ISceneService
{
public:
    ReplicationManager(Scene* scene, bool isServer)
        : _isServer(isServer),
        _scene(scene)
    {
        
    }

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

    void Client_ReceiveUpdateResponse(SLNet::BitStream& stream);
    void Client_SendUpdateRequest(float deltaTime, NetworkManager* networkManager);
    void Client_AddPlayerCommand(const PlayerCommand& command);

    void Server_ProcessUpdateRequest(SLNet::BitStream& message, SLNet::BitStream& response, int clientId);

    WorldState GetCurrentWorldState();

    std::unordered_set<NetComponent*> components;
    int localClientId = -1;

private:
    void ReceiveEvent(const IEntityEvent& ev) override;

    void ProcessSpawnEntity(ReadWriteBitStream& stream);
    void ProcessDestroyEntity(ReadWriteBitStream& stream);
    void ProcessEntitySnapshotMessage(ReadWriteBitStream& stream);

    WorldState* GetWorldSnapshot(uint32 snapshotId);

    std::unordered_map<int, NetComponent*> _componentsByNetId;
    std::unordered_map<int, ClientState> _clientStateByClientId;

    int _nextNetId = 0;
    bool _isServer;
    Scene* _scene;
    float _sendUpdateTimer = 0;

    uint32 _currentSnapshotId = 0;
    CircularQueue<WorldState, 32> _worldSnapshots;
};