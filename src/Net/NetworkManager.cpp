#include "NetworkManager.hpp"

#include "Engine.hpp"
#include "slikenet/peerinterface.h"
#include "slikenet/MessageIdentifiers.h"
#include "System/Logger.hpp"
#include "Tools/Console.hpp"

enum GameMessages
{
	ID_SERVER_PACKET = ID_USER_PACKET_ENUM + 1,
	ID_CLIENT_PACKET
};

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
			_isConnectedToServer = true;;
		}
	}

	for (packet = _peerInterface->Receive(); packet; _peerInterface->DeallocatePacket(packet), packet = _peerInterface->Receive())
	{
		switch (packet->data[0])
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			printf("Another client has disconnected.\n");
			break;
		case ID_REMOTE_CONNECTION_LOST:
			printf("Another client has lost the connection.\n");
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			printf("Another client has connected.\n");
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			printf("Our connection request has been accepted.\n");
			break;
		case ID_NEW_INCOMING_CONNECTION:
			printf("A connection is incoming.\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("The server is full.\n");
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			if (_isServer) {
				printf("A client has disconnected.\n");
			}
			else {
				printf("We have been disconnected.\n");
			}
			break;
		case ID_CONNECTION_LOST:
			if (_isServer) {
				printf("A client lost the connection.\n");
			}
			else {
				printf("Connection lost.\n");
			}
			break;

		case ID_SERVER_PACKET:
		{
			if (onReceiveMessageFromClient)
			{
				SLNet::BitStream response;
				response.Write((SLNet::MessageID)ID_CLIENT_PACKET);
				SLNet::BitStream message(packet->data, packet->length, false);
				message.IgnoreBytes(sizeof(SLNet::MessageID));		// Skip message header

				onReceiveMessageFromClient(message, response);

				_peerInterface->Send(&response, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);

			}

			break;
		}

		case ID_CLIENT_PACKET:
		{
			SLNet::BitStream message(packet->data, packet->length, false);
			message.IgnoreBytes(sizeof(SLNet::MessageID));		// Skip message header
			onReceiveServerResponse(message);

			break;
		}

		default:
			printf("Message with identifier %i has arrived.\n", packet->data[0]);
			break;
		}
	}
}

void NetworkManager::ConnectToServer(const char* serverAddress)
{
	_serverAddress = serverAddress;

	Log("Connecting to server...\n");
	auto connectResult = _peerInterface->Connect("127.0.0.1", Port, nullptr, 0);

	if (connectResult != SLNet::CONNECTION_ATTEMPT_STARTED)
	{
		FatalError("Failed to initiate server connection");
	}
}

void NetworkManager::SendPacketToServer(const std::function<void(SLNet::BitStream&)>& writeFunc)
{
	SLNet::BitStream stream;
	stream.Write((SLNet::MessageID)ID_SERVER_PACKET);
	writeFunc(stream);
	_peerInterface->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, SLNet::AddressOrGUID(SLNet::SystemAddress("127.0.0.1", Port)), false);
}
