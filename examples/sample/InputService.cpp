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
        scene->GetCameraFollower()->CenterOn(self->Center());
        activePlayer = self;
        players.push_back(self);
    }
    else if(auto connectedEvent = ev.Is<PlayerConnectedEvent>())
    {
        auto player = static_cast<PlayerEntity*>(scene->CreateEntity({
            EntityProperty::EntityType<PlayerEntity>(),
            { "position",connectedEvent->position.has_value() ? connectedEvent->position.value() : Vector2( 1819.0f, 2023.0f) },
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
            unsigned int keyBits;
            message.Read(keyBits);

            auto player = GetPlayerByNetId(clientId);
            if(player != nullptr)
            {
                player->keyBits = keyBits;
            }

            response.Write(PacketType::UpdateResponse);
            response.Write((int)players.size());

            for(auto player : players)
            {
                response.Write(player->netId);
                response.Write(player->Center().x);
                response.Write(player->Center().y);
            }
        };
    }
    else
    {
        net->onUpdateResponse = [=](SLNet::BitStream& message)
        {
            int totalPlayerUpdates = 0;
            message.Read(totalPlayerUpdates);

            for(int i = 0; i < totalPlayerUpdates; ++i)
            {
                int id;
                message.Read(id);

                Vector2 position;
                message.Read(position.x);
                message.Read(position.y);

                PlayerEntity* self;
                if(activePlayer.TryGetValue(self))
                {
                    //if (self->netId == id) continue;
                }

                auto player = GetPlayerByNetId(id);

                if(player == nullptr)
                {
                    scene->SendEvent(PlayerConnectedEvent(id, position));
                }
                else
                {
                    player->SetCenter(position);
                }
            }
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
            unsigned int keyBits = (g_leftButton.IsDown() << 0)
                | (g_rightButton.IsDown() << 1)
                | (g_upButton.IsDown() << 2)
                | (g_downButton.IsDown() << 3);

            net->SendPacketToServer([=](SLNet::BitStream& message)
            {
                message.Write(PacketType::UpdateRequest);
                message.Write(keyBits);
            });
        }
    }
    else
    {
        for (auto player : players)
        {
            Vector2 moveDirection;

            if (player->keyBits & 1) moveDirection.x -= 1;
            if (player->keyBits & 2) moveDirection.x += 1;
            if (player->keyBits & 4) moveDirection.y -= 1;
            if (player->keyBits & 8) moveDirection.y += 1;

            player->SetMoveDirection(moveDirection * 300);
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