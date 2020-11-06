
#include "InputService.hpp"

#include <slikenet/MessageIdentifiers.h>
#include <slikenet/PacketPriority.h>
#include <slikenet/socket2.h>
#include <slikenet/BitStream.h>
#include <slikenet/peerinterface.h>

#include "Engine.hpp"
#include "Components/RigidBodyComponent.hpp"
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

ConsoleVar<bool> autoConnect("auto-connect", false);

/*

[x] In every command, on the server, store where the player is at the end of that command
[x] The server shouldn't delete a command when it's done. It should mark it as complete
[x] When returning the snapshot to a client, rewind all the other players to the time of the snapshot (the time when the
    current command started) by lerping between that position and the previous position

[ ] Add a snapshot buffer to the players on the client that stores where they were given a command id
[ ] Each snapshot should have a game time of when that took snapshot happened
[ ] When rendering, use the current game time and snapshot buffer to determine where to draw the player

*/

void InputService::ReceiveEvent(const IEntityEvent& ev)
{
    auto net = scene->GetEngine()->GetNetworkManger();

    if(ev.Is<SceneLoadedEvent>())
    {
        if(net->IsClient() && autoConnect.Value())
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
            ++currentFixedUpdateId;

            for (auto player : players)
            {
                int physicsTime = Scene::PhysicsDeltaTime * 1000;

                PlayerCommand* commandToExecute = nullptr;


                for (auto& currentCommand : player->commands)
                {
                    if (currentCommand.status == PlayerCommandStatus::Complete)
                    {
                        continue;
                    }
                    else
                    {
                        commandToExecute = &currentCommand;
                        break;
                    }
                }

                if (commandToExecute != nullptr)
                {
                    if (commandToExecute->status == PlayerCommandStatus::NotStarted)
                    {
                        commandToExecute->fixedUpdateStartId = currentFixedUpdateId;
                        commandToExecute->status = PlayerCommandStatus::InProgress;
                        commandToExecute->positionAtStartOfCommand = player->Center();
                    }

                    auto direction = GetDirectionFromKeyBits(commandToExecute->keys) * 300;
                    player->SetMoveDirection(direction);

                    if ((int)commandToExecute->fixedUpdateCount - 1 <= 0)
                    {
                        commandToExecute->status = PlayerCommandStatus::Complete;
                        
                        player->lastServedExecuted = commandToExecute->id;
                        player->positionAtStartOfCommand = player->Center();

                    }
                    else
                    {
                        commandToExecute->fixedUpdateCount--;
                    }

                    totalTime += physicsTime;
                }
            }
        }
        else
        {
            ++fixedUpdateCount;
        }

        ++currentFixedUpdateId;
    }
    else if(auto joinedServerEvent = ev.Is<JoinedServerEvent>())
    {
        auto self = static_cast<PlayerEntity*>(scene->CreateEntity({
            EntityProperty::EntityType<PlayerEntity>(),
            { "position", { 1819.0f, 2023.0f } },
            { "dimensions", { 32, 32 } },
        }));

        self->netId = joinedServerEvent->selfId;
        self->isClientPlayer = true;
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

        if (net->IsServer())
        {
            scene->GetCameraFollower()->FollowEntity(player);
            scene->GetCameraFollower()->CenterOn(player->Center());
        }
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
                                message.Read(newCommand.fixedUpdateCount);
                                player->lastServerSequenceNumber = currentId;

                                if (player->commands.IsFull())
                                {
                                    player->commands.Dequeue();
                                }

                                player->commands.Enqueue(newCommand);
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

                PlayerCommand* lastCommand = player->GetCommandById(player->lastServedExecuted);
                int clientCommandStartTime = lastCommand != nullptr
                    ? lastCommand->fixedUpdateStartId
                    : currentFixedUpdateId - 1;

                for (auto p : players)
                {
                    response.Write(p->netId);

                    Vector2 position;
                    
                    if(p == player)
                    {
                        position = p->positionAtStartOfCommand;
                    }
                    else
                    {
                        position = p->Center(); //p->PositionAtFixedUpdateId(clientCommandStartTime, currentFixedUpdateId);
                    }

                    response.Write(position.x);
                    response.Write(position.y);
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

                auto lastCommandExecuted = self->GetCommandById(lastExecuted);

                if(player == nullptr)
                {
                    scene->SendEvent(PlayerConnectedEvent(id, position));
                }
                else if(player != self)
                {
                    //player->SetCenter(position);
                    
                    PlayerSnapshot snapshot;
                    snapshot.commandId = lastExecuted;
                    snapshot.position = position;
                    snapshot.time = lastCommandExecuted->timeRecorded;
                    player->AddSnapshot(snapshot);
                }
                else
                {
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
        PlayerEntity* self;
        if (activePlayer.TryGetValue(self))
        {
            unsigned int keyBits = (g_leftButton.IsDown() << 0)
                | (g_rightButton.IsDown() << 1)
                | (g_upButton.IsDown() << 2)
                | (g_downButton.IsDown() << 3);


            if (fixedUpdateCount > 0)
            {
                PlayerCommand command;
                command.id = ++self->nextCommandSequenceNumber;
                command.keys = keyBits;
                command.fixedUpdateCount = fixedUpdateCount;
                command.timeRecorded = scene->timeSinceStart;
                fixedUpdateCount = 0;

                self->commands.Enqueue(command);
            }

            // Time to send new update to server with missing commands
            if (sendUpdateTimer <= 0)
            {
                sendUpdateTimer = 1.0 / 30;
                std::vector<PlayerCommand> missingCommands;

                while (!self->commands.IsEmpty() && self->commands.Peek().id < self->lastServedExecuted)
                {
                    self->commands.Dequeue();
                }

                for(auto& command : self->commands)
                {
                    if (command.id > self->lastServerSequenceNumber)
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
                            message.Write(missingCommands[i].fixedUpdateCount);
                        }
                    }
                });
            }

            Vector2 offset;
            // Update position based on prediction
            self->SetCenter(self->positionAtStartOfCommand);
            //player->rigidBody->body->SetTransform(Scene::PixelToBox2D(player->positionAtStartOfCommand), 0);

            // Lock other players
            for(auto player : players)
            {
                if(player != self)
                {
                    player->rigidBody->body->SetLinearVelocity(b2Vec2(0, 0));
                }
            }

            // Update client side prediction
            for (auto& command : self->commands)
            {
                if ((int)command.id > (int)self->lastServedExecuted)
                {
                    for(int i = 0; i < (int)command.fixedUpdateCount; ++i)
                    {
                        self->SetMoveDirection(GetDirectionFromKeyBits(command.keys) * 300);
                        scene->ForceFixedUpdate();
                    }
                }
            }

            // Unlock other players
            for (auto player : players)
            {
                if (player != self)
                {
                    //player->rigidBody->body->SetType(b2_dynamicBody);
                }
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

    renderer->RenderString(FontSettings(ResourceManager::GetResource<SpriteFont>("console-font"_sid), 1),
        status.c_str(),
        Vector2(0, 200) + scene->GetCamera()->TopLeft(),
        -1);
}