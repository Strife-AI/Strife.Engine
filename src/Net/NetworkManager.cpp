#include "NetworkManager.hpp"

#include "Engine.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneManager.hpp"
#include "slikenet/peerinterface.h"
#include "slikenet/MessageIdentifiers.h"
#include "System/Logger.hpp"
#include "Tools/Console.hpp"

NetworkManager::NetworkManager(bool isServer)
    : _isServer(isServer)
{   
    _peerInterface = SLNet::RakPeerInterface::GetInstance();

    if (_isServer)
    {
        Log("Running as server\n");
        SLNet::SocketDescriptor sd(Port, nullptr);
        auto result = _peerInterface->Startup(MaxPlayers, &sd, 1);

		if(result != SLNet::RAKNET_STARTED)
		{
			FatalError("Failed to startup server");
		}

        _peerInterface->SetMaximumIncomingConnections(MaxPlayers);
    }
    else
    {
        Log("Running as client\n");
		SLNet::SocketDescriptor sd;
        auto result = _peerInterface->Startup(1, &sd, 1);

		if (result != SLNet::RAKNET_STARTED)
		{
			FatalError("Failed to startup client");
		}
    }
}

NetworkManager::~NetworkManager()
{
    Log("Shutting down network manager\n");
    SLNet::RakPeerInterface::DestroyInstance(_peerInterface);
}

namespace SLNet
{
	const RakNetGUID UNASSIGNED_RAKNET_GUID((uint64_t)-1);
}

void ConnectCommand(ConsoleCommandBinder& binder)
{
	std::string address;

	binder
		.Bind(address, "serverAddress")
		.Help("Connects to a server");

	Engine::GetInstance()->GetNetworkManger()->ConnectToServer(address.c_str());
}

ConsoleCmd connectCmd("connect", ConnectCommand);

void NetworkManager::Update()
{
	SLNet::Packet* packet;
	if (!_isConnectedToServer && IsClient())
	{
		auto state = _peerInterface->GetConnectionState(SLNet::AddressOrGUID(SLNet::SystemAddress(_serverAddress.c_str(), Port)));

		if(state == SLNet::IS_CONNECTED)
		{
			_isConnectedToServer = true;
		}
	}

	for (packet = _peerInterface->Receive(); packet; _peerInterface->DeallocatePacket(packet), packet = _peerInterface->Receive())
	{
		SLNet::BitStream message(packet->data, packet->length, false);
		message.IgnoreBytes(sizeof(SLNet::MessageID));		// Skip message header

		PacketType type = (PacketType)packet->data[0];

		if(_isServer)
		{
			SLNet::BitStream response;
			if(ProcessServerPacket(message, type, packet, response))
			{
				_peerInterface->Send(&response, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
			}
		}
		else
		{
			ProcessClientPacket(message, type);
		}
	}
}

void NetworkManager::ConnectToServer(const char* serverAddress)
{
	_serverAddress = serverAddress;

	Log("Connecting to server...\n");
	auto connectResult = _peerInterface->Connect(serverAddress, Port, nullptr, 0);

	if (connectResult != SLNet::CONNECTION_ATTEMPT_STARTED)
	{
		FatalError("Failed to initiate server connection");
	}
}

void NetworkManager::SendPacketToServer(const std::function<void(SLNet::BitStream&)>& writeFunc)
{
	SLNet::BitStream stream;
	writeFunc(stream);
	_peerInterface->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, SLNet::AddressOrGUID(SLNet::SystemAddress(_serverAddress.c_str(), Port)), false);
}

bool NetworkManager::ProcessServerPacket(SLNet::BitStream& message, PacketType type, SLNet::Packet* packet, SLNet::BitStream& response)
{
	switch(type)
	{
	    case PacketType::NewConnection:
		    ++_clientId;
			_clientIdByGuid[SLNet::RakNetGUID::ToUint32(packet->guid)] = _clientId;

		    response.Write(PacketType::NewConnectionResponse);
		    response.Write(_clientId);
			GetScene()->SendEvent(PlayerConnectedEvent(_clientId));
		    return true;

		case PacketType::UpdateRequest:
			onUpdateRequest(message, response, _clientIdByGuid[SLNet::RakNetGUID::ToUint32(packet->guid)]);
			return true;
	}

	return false;
}

void NetworkManager::ProcessClientPacket(SLNet::BitStream& message, PacketType type)
{
	switch(type)
	{
	    case PacketType::NewConnectionResponse:
			message.Read(_clientId);
			Log("Assigned client id %d\n", _clientId);
			GetScene()->SendEvent(JoinedServerEvent(_clientId));
			break;

		case PacketType::UpdateResponse:
			onUpdateResponse(message);
			break;
	}
}

Scene* NetworkManager::GetScene()
{
	return Engine::GetInstance()->GetSceneManager()->GetScene();
}
