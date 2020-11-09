#pragma once
#include <unordered_map>
#include <unordered_set>



#include "Components/NetComponent.hpp"
#include "Math/Vector2.hpp"
#include "Scene/Entity.hpp"

namespace SLNet {
    class BitStream;
}

enum class MessageType : uint8;
struct ReadWriteBitStream;

class ReplicationManager
{
public:
    void AddNetComponent(NetComponent* component)
    {
        if (_isServer)
        {
            component->netId = _nextNetId++;
            _componentsByNetId[component->netId] = component;
        }
    }

    void RemoveNetComponent(NetComponent* component)
    {
        _componentsByNetId.erase(component->netId);
    }

    void UpdateClient(SLNet::BitStream& stream);

    std::unordered_set<NetComponent*> components;

private:
    void ProcessSpawnEntity(ReadWriteBitStream& stream);
    void ProcessEntitySnapshotMessage(ReadWriteBitStream& stream);

    std::unordered_map<int, NetComponent*> _componentsByNetId;

    int _nextNetId = 0;
    bool _isServer; // TODO
    Scene* _scene;  // TODO
};