#pragma once
#include <list>
#include <memory>
#include <queue>
#include <gsl/span>
#include <slikenet/BitStream.h>
#include <slikenet/MessageIdentifiers.h>
#include <slikenet/peerinterface.h>


#include "Scene/SceneManager.hpp"

namespace SLNet {
    class BitStream;
}

enum class PacketType : unsigned char
{
    NewConnection = (unsigned char)ID_NEW_INCOMING_CONNECTION,

    NewConnectionResponse = (unsigned char)ID_USER_PACKET_ENUM + 1,
    UpdateRequest,
    UpdateResponse
};

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
            auto packet = raknetInterface->AllocatePacket(data.size_bytes());
            memcpy(packet->data, data.data(), data.size_bytes());
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
            auto packet = raknetInterface->AllocatePacket(data.size_bytes());
            memcpy(packet->data, data.data(), data.size_bytes());
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

    void SetLocalAddress(NetworkInterface* localInterface_, SLNet::AddressOrGUID localAddress_)
    {
        localInterface = localInterface_;
        localAddress = localAddress_;
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
    int clientId;
};

struct BaseGameInstance
{
    BaseGameInstance(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_, bool isServer)
        : sceneManager(engine, isServer),
        networkInterface(raknetInterface),
        localAddress(localAddress_),
        engine(engine)
    {

    }

    void RunFrame();
    void Render(Scene* scene, float deltaTime, float renderDeltaTime);

    virtual void UpdateNetwork() = 0;

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
        : BaseGameInstance(engine, raknetInterface, localAddress_, true)
    {
        isHeadless = true;
    }

    void UpdateNetwork() override;

    ServerGameClient clients[MaxClients];
};

struct ClientGame : BaseGameInstance
{
    static constexpr int MaxClients = 8;

    ClientGame(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_)
        : BaseGameInstance(engine, raknetInterface, localAddress_, false)
    {
        isHeadless = false;
    }

    void UpdateNetwork() override;

    ServerGameClient clients[MaxClients];
};
