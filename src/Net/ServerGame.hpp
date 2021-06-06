#pragma once

#include <functional>
#include <list>
#include <memory>
#include <queue>
#include <gsl/span>
#include <unordered_map>
#include <slikenet/BitStream.h>
#include <slikenet/MessageIdentifiers.h>
#include <slikenet/peerinterface.h>
#include "Math/Vector2.hpp"

#include "Scene/SceneManager.hpp"
#include "Memory/StringId.hpp"
#include "FileTransfer.hpp"

namespace SLNet
{
    class BitStream;
}

enum class PacketType : unsigned char
{
    NewConnection = (unsigned char)ID_NEW_INCOMING_CONNECTION,
    ConnectionAttemptFailed = (unsigned char)ID_CONNECTION_ATTEMPT_FAILED,
    Disconnected = (unsigned char)ID_DISCONNECTION_NOTIFICATION,
    ConnectionLost = (unsigned char)ID_CONNECTION_LOST,

    NewConnectionResponse = (unsigned char)ID_USER_PACKET_ENUM + 1,
    UpdateRequest,
    UpdateResponse,
    ServerFull,
    Ping,
    PingResponse,
    RpcCall,
    UploadFile
};

enum class ClientConnectionStatus
{
    Connected,
    NotConnected
};


struct ReadWriteBitStream
{
    ReadWriteBitStream(SLNet::BitStream& stream_, bool isReading_)
        : stream(stream_),
          isReading(isReading_)
    {

    }

    template<typename T>
    ReadWriteBitStream& Add(T& out)
    {
        if (isReading) stream.Read(out);
        else stream.Write(out);

        return *this;
    }

    ReadWriteBitStream& Add(Vector2& out);
    ReadWriteBitStream& Add(std::string& str);
    ReadWriteBitStream& Add(std::vector<unsigned char>& v);

    SLNet::BitStream& stream;
    bool isReading;
};

struct IRemoteProcedureCall
{
    virtual void Serialize(ReadWriteBitStream& stream)
    {
    }

    virtual void Execute() = 0;
    virtual const char* GetName() const = 0;
};

template<typename T>
constexpr const char* GetRpcName();

template<typename T>
struct RemoteProcedureCall : IRemoteProcedureCall
{
    const char* GetName() const override
    {
        return GetRpcName<T>();
    }

    static void ExecuteInternal(ReadWriteBitStream& stream, Engine* engine, int fromClientId, SLNet::AddressOrGUID fromAddress)
    {
        T rpc;
        rpc.engine = engine;
        rpc.fromClientId = fromClientId;
        rpc.fromAddress = fromAddress;
        rpc.Serialize(stream);
        rpc.Execute();
    }

    Engine* engine;
    int fromClientId;
    SLNet::AddressOrGUID fromAddress;
};

#define DEFINE_RPC(structName_) struct structName_; \
    template<> inline constexpr const char* GetRpcName<structName_>() { return #structName_; } \
    struct structName_ : RemoteProcedureCall<structName_>

struct NetworkInterface
{
    NetworkInterface(SLNet::RakPeerInterface* raknetInterface_)
        : raknetInterface(raknetInterface_)
    {

    }

    void SendUnreliable(const SLNet::AddressOrGUID& address, gsl::span<unsigned char> data)
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
        if (lastPacket != nullptr)
        {
            raknetInterface->DeallocatePacket(lastPacket);
        }

        if (!localPackets.empty())
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

class RpcManager
{
public:
    RpcManager(NetworkInterface* networkInterface)
        : _networkInterface(networkInterface)
    {

    }

    template<typename T>
    void Register()
    {
        // TODO: check for duplicates
        _handlersByStringIdName[StringId(GetRpcName<T>()).key] = T::ExecuteInternal;
    }

    void Execute(SLNet::AddressOrGUID address, const IRemoteProcedureCall& rpc);

    void Receive(SLNet::BitStream& stream, Engine* engine, int fromClientId, SLNet::AddressOrGUID fromAddress);

private:
    std::unordered_map<unsigned int, void (*)(ReadWriteBitStream& stream, Engine* engine, int clientId, SLNet::AddressOrGUID)> _handlersByStringIdName;
    NetworkInterface* _networkInterface;
};

struct ServerGameClient
{
    ClientConnectionStatus status = ClientConnectionStatus::NotConnected;
    SLNet::AddressOrGUID address;
    int clientId;
};

class FileTransferService;

struct BaseGameInstance
{
    virtual ~BaseGameInstance() = default;

    BaseGameInstance(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_,
        bool isServer);

    void RunFrame(float currentTime);
    void Render(Scene* scene, float deltaTime, float renderDeltaTime);

    Scene* GetScene()
    {
        return sceneManager.GetScene();
    }

    virtual void UpdateNetwork() = 0;

    virtual void PostUpdateEntities()
    {
    }

    SceneManager sceneManager;
    NetworkInterface networkInterface;
    RpcManager rpcManager;
    SLNet::AddressOrGUID localAddress;
    Engine* engine;
    bool isHeadless = false;
    float targetTickRate;
    float nextUpdateTime = 0;
    FileTransferService fileTransferService;
	std::chrono::steady_clock::time_point lastRenderTime;
};

struct ServerGame : BaseGameInstance
{
    static constexpr int MaxClients = 8;

    ServerGame(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_);

    void HandleNewConnection(SLNet::Packet* packet);
    void UpdateNetwork() override;
    void PostUpdateEntities() override;
    int AddClient(const SLNet::AddressOrGUID& address);
    int GetClientId(const SLNet::AddressOrGUID& address);
    void ForEachClient(const std::function<void(ServerGameClient&)>& handler);
    void BroadcastRpc(const IRemoteProcedureCall& rpc);
    void ExecuteRpc(int clientId, const IRemoteProcedureCall& rpc);
    int TotalConnectedClients();

    ServerGameClient clients[MaxClients];

    void DisconnectClient(int clientId);
};

struct ClientGame : BaseGameInstance
{
    ClientGame(Engine* engine, SLNet::RakPeerInterface* raknetInterface, SLNet::AddressOrGUID localAddress_);

    void Disconnect();

    void UpdateNetwork() override;
    void MeasureRoundTripTime();
    float GetServerClockOffset();
    void AddPlayerCommand(struct PlayerCommand& command);

    int clientId = -1;
    SLNet::AddressOrGUID serverAddress;
    std::vector<float> pingBuffer;
};

DEFINE_RPC(ServerSetPlayerInfoRpc)
{
    ServerSetPlayerInfoRpc()
    {

    }

    ServerSetPlayerInfoRpc(const std::string& name)
        : name(name)
    {

    }

    void Serialize(ReadWriteBitStream& rw) override
    {
        rw.Add(name);
    }

    void Execute() override;

    std::string name;
};

DEFINE_RPC(ClientSetPlayerInfoRpc)
{
    ClientSetPlayerInfoRpc()
    {

    }

    ClientSetPlayerInfoRpc(int clientId, const std::string& name)
        : clientId(clientId),
          name(name)
    {

    }

    void Serialize(ReadWriteBitStream& rw) override
    {
        rw.Add(clientId).Add(name);
    }

    void Execute() override;

    int clientId;
    std::string name;
};
