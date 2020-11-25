#pragma once
#include <functional>
#include <optional>
#include <slikenet/BitStream.h>
#include <slikenet/MessageIdentifiers.h>


#include "Scene/IEntityEvent.hpp"

enum class PacketType : unsigned char;

namespace SLNet
{
    class RakPeerInterface;
}

DEFINE_EVENT(PlayerConnectedEvent)
{
    PlayerConnectedEvent(int id_, std::optional<Vector2> = std::nullopt)
        : id(id_)
    {
        
    }

    int id;
    std::optional<Vector2> position;
};

DEFINE_EVENT(JoinedServerEvent)
{
    JoinedServerEvent(int selfId_)
        : selfId(selfId_)
    {
        
    }

    int selfId;
};


class NetworkManager
{
public:
    static constexpr int MaxPlayers = 4;
    static constexpr int Port = 60000;

    static constexpr int ShutdownTimeInMilliseconds = 60;

    NetworkManager(bool isServer);
    ~NetworkManager();

    void Update();

    bool IsServer() const { return _isServer; }
    bool IsClient() const { return !_isServer; }

    void ConnectToServer(const char* serverAddress);

    SLNet::RakPeerInterface* GetPeerInterface() const { return _peerInterface; }

    void SendPacketToServer(const std::function<void(SLNet::BitStream&)>& writeFunc);

private:
    bool ProcessServerPacket(SLNet::BitStream& message, PacketType type, SLNet::Packet* packet, SLNet::BitStream& response);
    void ProcessClientPacket(SLNet::BitStream& message, PacketType type);

    Scene* GetScene();

    bool _isServer = false;
    bool _isConnectedToServer = false;
    std::string _serverAddress;
    int _clientId = -1;

    int _nextClientId = 0;

    SLNet::RakPeerInterface* _peerInterface;
    std::unordered_map<unsigned int, int> _clientIdByGuid;
};
