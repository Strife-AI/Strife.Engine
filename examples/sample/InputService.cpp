
#include "InputService.hpp"

#include <slikenet/MessageIdentifiers.h>
#include <slikenet/PacketPriority.h>
#include <slikenet/socket2.h>
#include <slikenet/BitStream.h>
#include <slikenet/peerinterface.h>

#include "Engine.hpp"
#include "Memory/Util.hpp"
#include "Renderer/Renderer.hpp"
#include "Tools/Console.hpp"

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

    if(ev.Is<SceneLoadedEvent>())
    {
        if(net->IsClient())
        {
            Sleep(2000);
            scene->GetEngine()->GetConsole()->Execute("connect 127.0.0.1");
        }
    }
    if (ev.Is<PreUpdateEvent>())
    {
        HandleInput();
    }
    else if(auto renderEvent = ev.Is<RenderEvent>())
    {
        if (net->IsServer())
        {
            status.VFormat("Total time: %d", totalTime);
            totalTime = 0;
        }

        Render(renderEvent->renderer);
    }
    else if (auto fixedUpdateEvent = ev.Is<FixedUpdateEvent>())
    {
        if (net->IsServer())
        {
            for (auto player : players)
            {
                int physicsTime = Scene::PhysicsDeltaTime * 1000;

                if (player->commands.size() > 0)
                {
                    auto& currentCommand = player->commands.front();

                    auto direction = GetDirectionFromKeyBits(currentCommand.keys) * 300;
                    player->SetMoveDirection(direction);

                    if ((int)currentCommand.timeMilliseconds - 1 <= 0)
                    {
                        player->lastServedExecuted = currentCommand.id;
                        player->commands.pop_front();
                        player->positionAtStartOfCommand = player->Center();
                    }
                    else
                    {
                        currentCommand.timeMilliseconds--;
                    }

                    totalTime += physicsTime;
                }
            }
        }
        else
        {
            ++fixedUpdateCount;
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
        scene->GetCameraFollower()->FollowEntity(player);
        scene->GetCameraFollower()->CenterOn(player->Center());
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

                response.Write(player->clientClock);

                response.Write(player->lastServerSequenceNumber);

                response.Write(player->lastServedExecuted);
                response.Write((int)players.size());

                for (auto player : players)
                {
                    response.Write(player->netId);
                    response.Write(player->positionAtStartOfCommand.x);
                    response.Write(player->positionAtStartOfCommand.y);
                }
            }
        };
    }
    else
    {
        net->onUpdateResponse = [=](SLNet::BitStream& message)
        {
            int clientClock;
            message.Read(clientClock);

            int lastServerSequence;
            message.Read(lastServerSequence);

            int lastExecuted;
            message.Read(lastExecuted);

            int totalPlayerUpdates = 0;
            message.Read(totalPlayerUpdates);

            PlayerEntity* self;
            if (activePlayer.TryGetValue(self))
            {
                self->lastServerSequenceNumber = lastServerSequence;
                self->lastServedExecuted = lastExecuted;

                //while (self->commands.size() > 0 && self->commands.front().id <= lastExecuted)
                //{
                //    self->commands.pop_front();
                //}

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
                else if(player != self)
                {
                    player->SetCenter(position);
                }
                else
                {
                    // Reapply the moves we locally made on the client
                    Vector2 offset;
                    int clock = 0;
                    bool firstIntersection = false;
                    int totalMs = 0;

                    printf("============\n");

                    self->positionAtStartOfCommand = position;
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

            if (fixedUpdateCount > 0)
            {
                PlayerCommand command;
                command.id = ++player->nextCommandSequenceNumber;
                command.keys = keyBits;
                command.timeMilliseconds = fixedUpdateCount;
                fixedUpdateCount = 0;

                player->commands.push_back(command);

                //player->SetMoveDirection(direction * 300);
            }

            // Time to send new update to server with missing commands
            if (sendUpdateTimer <= 0)
            {
                sendUpdateTimer = 1.0 / 30;
                std::vector<PlayerCommand> missingCommands;

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

            Vector2 offset;
            // Update position based on prediction
            for (auto& command : player->commands)
            {
                if ((int)command.id > (int)player->lastServedExecuted)
                {
                    //totalMs += command.timeMilliseconds * 5;
                    offset += GetDirectionFromKeyBits(command.keys) * 300 * (command.timeMilliseconds * 5 / 1000.0);
                }
            }

            //status.VFormat("Offset: %f %f, total time: %d", offset.x, offset.y, totalMs);

            player->SetCenter(player->positionAtStartOfCommand + offset);
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

    renderer->RenderString(FontSettings(ResourceManager::GetResource<SpriteFont>("console-font"_sid), 1),
        status.c_str(),
        Vector2(0, 200) + scene->GetCamera()->TopLeft(),
        -1);
}