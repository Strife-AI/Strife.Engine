
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

Vector2 GetDirectionFromKeyBits(unsigned int keyBits)
{
    Vector2 moveDirection;

    if (keyBits & 1) moveDirection.x -= 1;
    if (keyBits & 2) moveDirection.x += 1;
    if (keyBits & 4) moveDirection.y -= 1;
    if (keyBits & 8) moveDirection.y += 1;

    return moveDirection;
}

void InputService::ReceiveEvent(const IEntityEvent& ev)
{
    auto net = scene->GetEngine()->GetNetworkManger();

    if (ev.Is<PreUpdateEvent>())
    {
        HandleInput();
    }
    else if(auto renderEvent = ev.Is<RenderEvent>())
    {
        Render(renderEvent->renderer);
    }
    else if(auto fixedUpdateEvent = ev.Is<FixedUpdateEvent>())
    {
        if (net->IsServer())
        {
            for (auto player : players)
            {
                int physicsTime = Scene::PhysicsDeltaTime * 1000;

                if(player->commands.size() > 0)
                {
                    auto& currentCommand = player->commands.front();
                    printf("id: %d, QS: %d, K: %d, T: %d\n",
                        (int)currentCommand.id,
                        (int)player->commands.size(),
                        (int)currentCommand.keys,
                        (int)currentCommand.timeMilliseconds);

                    auto direction = GetDirectionFromKeyBits(currentCommand.keys) * 300;
                    player->SetMoveDirection(direction);

                    if((int)currentCommand.timeMilliseconds - physicsTime >= 0)
                    {
                        player->commands.pop_front();
                    }
                    else
                    {
                        currentCommand.timeMilliseconds -= physicsTime;
                    }
                }
            }
        }
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
            auto player = GetPlayerByNetId(clientId);
            if (player != nullptr)
            {
                unsigned char commandsInPacket = 0;
                message.Read(commandsInPacket);

                if(commandsInPacket > 0)
                {
                    unsigned int firstCommandId = 0;
                    message.Read(firstCommandId);

                    if (player->lastServerSequenceNumber + 1 <= firstCommandId)
                    {
                        auto currentId = firstCommandId;
                        for (int i = 0; i < commandsInPacket; ++i)
                        {
                            if (currentId > player->lastServerSequenceNumber)
                            {
                                PlayerCommand newCommand;
                                newCommand.id = currentId;
                                message.Read(newCommand.keys);
                                message.Read(newCommand.timeMilliseconds);
                                player->lastServerSequenceNumber = currentId;

                                player->commands.push_back(newCommand);
                            }

                            ++currentId;
                        }
                    }
                }

                response.Write(PacketType::UpdateResponse);
                response.Write(player->lastServerSequenceNumber);
                response.Write((int)players.size());

                for (auto player : players)
                {
                    response.Write(player->netId);
                    response.Write(player->Center().x);
                    response.Write(player->Center().y);
                }
            }
        };
    }
    else
    {
        net->onUpdateResponse = [=](SLNet::BitStream& message)
        {
            int lastServerSequence;
            message.Read(lastServerSequence);

            int totalPlayerUpdates = 0;
            message.Read(totalPlayerUpdates);

            PlayerEntity* self;
            if (activePlayer.TryGetValue(self))
            {
                self->lastServerSequenceNumber = lastServerSequence;
                //if (self->netId == id) continue;
            }

            for(int i = 0; i < totalPlayerUpdates; ++i)
            {
                int id;
                message.Read(id);

                Vector2 position;
                message.Read(position.x);
                message.Read(position.y);

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
    sendUpdateTimer -= scene->deltaTime;
    
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

            auto direction = GetDirectionFromKeyBits(keyBits);

            PlayerCommand command;
            command.id = ++player->nextCommandSequenceNumber;
            command.keys = keyBits;
            command.timeMilliseconds = scene->deltaTime * 1000;
            player->commands.push_back(command);

            // Time to send new update to server with missing commands
            if (sendUpdateTimer <= 0)
            {
                sendUpdateTimer = 1.0 / 30;
                std::vector<PlayerCommand> missingCommands;

                while(player->commands.size() > 0 && player->commands.front().id <= player->lastServerSequenceNumber)
                {
                    player->commands.pop_front();
                }

                for(auto& command : player->commands)
                {
                    if(command.id > player->lastServerSequenceNumber)
                    {
                        missingCommands.push_back(command);
                    }
                }

                if(missingCommands.size() > 60)
                {
                    missingCommands.resize(60);
                }

                net->SendPacketToServer([=](SLNet::BitStream& message)
                {
                    message.Write(PacketType::UpdateRequest);
                    message.Write((unsigned char)missingCommands.size());

                    if (missingCommands.size() > 0)
                    {
                        message.Write((unsigned int)missingCommands[0].id);
                        
                        for (int i = 0; i < missingCommands.size(); ++i)
                        {
                            message.Write(missingCommands[i].keys);
                            message.Write(missingCommands[i].timeMilliseconds);
                        }
                    }
                });
            }
        }
    }
    else
    {

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