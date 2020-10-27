#include "InputService.hpp"

#include <slikenet/MessageIdentifiers.h>
#include <slikenet/PacketPriority.h>
#include <slikenet/socket2.h>
#include <slikenet/BitStream.h>
#include <slikenet/peerinterface.h>


#include "Engine.hpp"
#include "Memory/Util.hpp"
#include "Renderer/Renderer.hpp"

InputButton g_quit = InputButton(SDL_SCANCODE_ESCAPE);
InputButton g_upButton(SDL_SCANCODE_W);
InputButton g_downButton(SDL_SCANCODE_S);
InputButton g_leftButton(SDL_SCANCODE_A);
InputButton g_rightButton(SDL_SCANCODE_D);
InputButton g_nextPlayer(SDL_SCANCODE_TAB);

void InputService::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<PreUpdateEvent>())
    {
        HandleInput();
    }
    else if(auto renderEvent = ev.Is<RenderEvent>())
    {
        Render(renderEvent->renderer);
    }
    else if(auto joinedServerEvent = ev.Is<JoinedServerEvent>())
    {
        auto self = static_cast<PlayerEntity*>(scene->CreateEntity({
            EntityProperty::EntityType<PlayerEntity>(),
            { "position", { 1819.0f, 2023.0f } },
            { "dimensions", { 32, 32 } },
        }));

        self->netId = joinedServerEvent->selfId;
        scene->GetCameraFollower()->FollowEntity(self);
        activePlayer = self;
        players.push_back(self);
    }
    else if(auto connectedEvent = ev.Is<PlayerConnectedEvent>())
    {
        auto player = static_cast<PlayerEntity*>(scene->CreateEntity({
            EntityProperty::EntityType<PlayerEntity>(),
            { "position", { 1819.0f, 2023.0f } },
            { "dimensions", { 32, 32 } },
            }));

        player->netId = connectedEvent->id;
        players.push_back(player);
    }
}

PlayerEntity* InputService::GetPlayerByNetId(int netId)
{
    for(auto player : players)
    {
        if(player->netId == netId)
        {
            return player;
        }
    }

    return nullptr;
}

void InputService::OnAdded()
{
    auto net = scene->GetEngine()->GetNetworkManger();
    if(net->IsServer())
    {
        scene->GetCameraFollower()->FollowMouse();

        net->onUpdateRequest = [=](SLNet::BitStream& message, SLNet::BitStream& response, int clientId)
        {
            Vector2 position;
            message.Read(position.x);
            message.Read(position.y);

            auto player = GetPlayerByNetId(clientId);
            if(player != nullptr)
            {
                player->SetCenter(position);
            }

            response.Write(PacketType::UpdateResponse);
        };
    }
    else
    {
        net->onUpdateResponse = [=](SLNet::BitStream& message)
        {

        };
    }
}

void InputService::HandleInput()
{
    auto net = scene->GetEngine()->GetNetworkManger();
    auto peer = net->GetPeerInterface();

    if (g_quit.IsPressed())
    {
        scene->GetEngine()->QuitGame();
    }

    if (net->IsClient())
    {
        PlayerEntity* player;
        if (activePlayer.TryGetValue(player))
        {
            auto moveDirection = Vector2(
                g_leftButton.IsDown() ? -1 : g_rightButton.IsDown() ? 1 : 0,
                g_upButton.IsDown() ? -1 : g_downButton.IsDown() ? 1 : 0);

            float moveSpeed = 300;

            player->SetMoveDirection(moveDirection * moveSpeed);

            net->SendPacketToServer([=](SLNet::BitStream& message)
            {
                message.Write(PacketType::UpdateRequest);
                message.Write(player->Center().x);
                message.Write(player->Center().y);
            });
        }
    }
}

void InputService::Render(Renderer* renderer)
{
    PlayerEntity* currentPlayer;
    if (activePlayer.TryGetValue(currentPlayer))
    {
        renderer->RenderRectangleOutline(currentPlayer->Bounds(), Color::White(), -1);
    }
}