#pragma once
#include <list>
#include <memory>
#include <queue>
#include <gsl/span>
#include <slikenet/BitStream.h>
#include <slikenet/peerinterface.h>


#include "Scene/SceneManager.hpp"

namespace SLNet {
    class BitStream;
}

enum class ClientConnectionStatus
{
    Connected,
    NotConnected
};

struct NetworkInterface
{
    NetworkInterface(SLNet::RakPeerInterface* raknetInterface_)
        : raknetInterface(raknetInterface_)
    {

    }

    void SendUnreliable(const SLNet::AddressOrGUID& address, gsl::span<unsigned char> data)
    {
        if(address == localAddress)
        {
            auto packet = raknetInterface->AllocatePacket(data.size());
            localInterface->AddLocalPacket(packet);
        }
        else
        {
            raknetInterface->Send(
                (char*)data.data(),
                data.size_bytes(),
                PacketPriority::HIGH_PRIORITY,
                PacketReliability::UNRELIABLE,
                0, 
                address, 
                false);
        }
    }

    void SendReliable(const SLNet::AddressOrGUID& address, gsl::span<unsigned char> data)
    {
        if (address == localAddress)
        {
            auto packet = raknetInterface->AllocatePacket(data.size());
            localInterface->AddLocalPacket(packet);
        }
        else
        {
            raknetInterface->Send(
                (char*)data.data(),
                data.size_bytes(),
                PacketPriority::HIGH_PRIORITY,
                PacketReliability::RELIABLE,
                0,
                address,
                false);
        }
    }

    bool TryGetPacket(SLNet::Packet*& outPacket)
    {
        if(lastPacket != nullptr)
        {
            raknetInterface->DeallocatePacket(lastPacket);
        }

        if(!localPackets.empty())
        {
            lastPacket = localPackets.front();
            localPackets.pop();
        }
        else
        {
            lastPacket = raknetInterface->Receive();
        }

        outPacket = lastPacket;
        return lastPacket != nullptr;
    }

    void SetLocalAddress()
    {
        
    }

    void AddLocalPacket(SLNet::Packet* packet)
    {
        localPackets.push(packet);
    }

    SLNet::RakPeerInterface* raknetInterface;
    NetworkInterface* localInterface;
    SLNet::AddressOrGUID localAddress;
    std::queue<SLNet::Packet*> localPackets;
    SLNet::Packet* lastPacket = nullptr;
};

struct ServerGameClient
{
    ClientConnectionStatus status = ClientConnectionStatus::NotConnected;
    SLNet::AddressOrGUID address;
    bool isLocalConnection;
    int clientId;
};

struct BaseGameInstance
{
    BaseGameInstance(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_)
        : sceneManager(engine),
        networkInterface(raknetInterface),
        localAddress(localAddress_),
        engine(engine)
    {

    }

    void RunFrame();
    void Render(Scene* scene, float deltaTime, float renderDeltaTime);

    SceneManager sceneManager;
    NetworkInterface networkInterface;
    SLNet::AddressOrGUID localAddress;
    Engine* engine;
    bool isHeadless = false;
    float targetTickRate;
};

struct ServerGame : BaseGameInstance
{
    static constexpr int MaxClients = 8;

    ServerGame(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_)
        : BaseGameInstance(engine, raknetInterface, localAddress_)
    {
        
    }

    ServerGameClient clients[MaxClients];
};
