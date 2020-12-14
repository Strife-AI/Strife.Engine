#pragma once
#include <functional>
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
    ConnectionAttemptFailed = (unsigned char)ID_CONNECTION_ATTEMPT_FAILED,

    NewConnectionResponse = (unsigned char)ID_USER_PACKET_ENUM + 1,
    UpdateRequest,
    UpdateResponse,
    ServerFull,
    Ping,
    PingResponse
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
        if(address == localAddress && localInterface != nullptr)
        {
            auto packet = raknetInterface->AllocatePacket(data.size_bytes());
            packet->systemAddress = localInterface->localAddress.systemAddress;

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
        if (address == localAddress && localInterface != nullptr)
        {
            auto packet = raknetInterface->AllocatePacket(data.size_bytes());
            packet->systemAddress = localInterface->localAddress.systemAddress;

            memcpy(packet->data, data.data(), data.size_bytes());
            localInterface->AddLocalPacket(packet);
        }
        else
        {
            raknetInterface->Send(
                (char*)data.data(),
                data.size_bytes(),
                PacketPriority::HIGH_PRIORITY,
                PacketReliability::RELIABLE_ORDERED_WITH_ACK_RECEIPT,
                0,
                address,
                false);
        }
    }

    void SendReliable(const SLNet::AddressOrGUID& address, SLNet::BitStream& stream)
    {
        SendReliable(address, gsl::span<unsigned char>(stream.GetData(), stream.GetNumberOfBytesUsed()));
    }

    void SendUnreliable(const SLNet::AddressOrGUID& address, SLNet::BitStream& stream)
    {
        SendUnreliable(address, gsl::span<unsigned char>(stream.GetData(), stream.GetNumberOfBytesUsed()));
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
    NetworkInterface* localInterface = nullptr;
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
    virtual ~BaseGameInstance() = default;

    BaseGameInstance(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_, bool isServer)
        : sceneManager(engine, isServer),
        networkInterface(raknetInterface),
        localAddress(localAddress_),
        engine(engine)
    {

    }

    void RunFrame(float currentTime);
    void Render(Scene* scene, float deltaTime, float renderDeltaTime);
    Scene* GetScene() { return sceneManager.GetScene(); }

    virtual void UpdateNetwork() = 0;
    virtual void PostUpdateEntities() { }

    SceneManager sceneManager;
    NetworkInterface networkInterface;
    SLNet::AddressOrGUID localAddress;
    Engine* engine;
    bool isHeadless = false;
    float targetTickRate;
    float nextUpdateTime = 0;
};

struct ServerGame : BaseGameInstance
{
    static constexpr int MaxClients = 8;

    ServerGame(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_)
        : BaseGameInstance(engine, raknetInterface, localAddress_, true)
    {
        isHeadless = true;
        targetTickRate = 30;
    }

    void HandleNewConnection(SLNet::Packet* packet);
    void UpdateNetwork() override;
    void PostUpdateEntities() override;
    int AddClient(const SLNet::AddressOrGUID& address);
    int GetClientId(const SLNet::AddressOrGUID& address);

    std::function<void(SLNet::BitStream& message, SLNet::BitStream& response, int clientId)> onUpdateRequest;

    ServerGameClient clients[MaxClients];
};

struct ClientGame : BaseGameInstance
{
    ClientGame(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_)
        : BaseGameInstance(engine, raknetInterface, localAddress_, false)
    {
        isHeadless = false;
        targetTickRate = 60;
    }

    void UpdateNetwork() override;
    void MeasureRoundTripTime();

    float GetServerClockOffset();

    std::function<void(SLNet::BitStream& message)> onUpdateResponse;
    int clientId = -1;
    SLNet::AddressOrGUID serverAddress;
    std::vector<float> pingBuffer;
};
