#pragma once
#include <functional>
#include <slikenet/BitStream.h>

namespace SLNet
{
    class RakPeerInterface;
}

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

    std::function<void(SLNet::BitStream& message, SLNet::BitStream& response)> onReceiveMessageFromClient;
    std::function<void(SLNet::BitStream& message)> onReceiveServerResponse;

private:
    bool _isServer = false;
    bool _isConnectedToServer = false;
    std::string _serverAddress;

    SLNet::RakPeerInterface* _peerInterface;
};