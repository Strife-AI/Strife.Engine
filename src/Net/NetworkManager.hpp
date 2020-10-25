#pragma once

class NetworkManager
{
public:
    NetworkManager(bool isServer)
        : _isServer(isServer)
    {
        
    }

    bool IsServer() const { return _isServer; }
    bool IsClient() const { return !_isServer; }

private:
    bool _isServer = false;
};