
#include "InputService.hpp"

#include <slikenet/BitStream.h>
#include <slikenet/peerinterface.h>

#include "Engine.hpp"
#include "PuckEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Memory/Util.hpp"
#include "Net/NetworkPhysics.hpp"
#include "Net/ReplicationManager.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"
#include "Tools/Console.hpp"

InputButton g_quit = InputButton(SDL_SCANCODE_ESCAPE);
InputButton g_upButton(SDL_SCANCODE_W);
InputButton g_downButton(SDL_SCANCODE_S);
InputButton g_leftButton(SDL_SCANCODE_A);
InputButton g_rightButton(SDL_SCANCODE_D);
InputButton g_nextPlayer(SDL_SCANCODE_TAB);

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

            //scene->GetCameraFollower()->FollowMouse();
            //scene->GetCameraFollower()->CenterOn({ 800, 800});
        }

        if(net->IsServer())
        {
            scene->CreateEntity({
                EntityProperty::EntityType<PuckEntity>(),
                { "position", { 1800, 1800} },
                { "dimensions", { 32, 32} },
            });
        }
    }
    if (ev.Is<UpdateEvent>())
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

        }
        else
        {
            ++fixedUpdateCount;
        }

        ++currentFixedUpdateId;
    }
    else if(auto joinedServerEvent = ev.Is<JoinedServerEvent>())
    {
        scene->replicationManager->localClientId = joinedServerEvent->selfId;
    }
    else if(auto connectedEvent = ev.Is<PlayerConnectedEvent>())
    {
        if (net->IsServer())
        {
            Vector2 positions[3] = {
                Vector2(2048 - 1000, 2048 - 1000),
                Vector2(2048 + 1000, 2048 + 1000),
                Vector2(2048 + 1000, 2048 - 1000),
            };

            Vector2 offsets[4] =
            {
                Vector2(-1, -1), Vector2(-1, 1), Vector2(1, -1), Vector2(1, 1)
            };

            for (auto offset : offsets)
            {
                auto position = positions[connectedEvent->id] + offset * 128;

                auto player = static_cast<PlayerEntity*>(scene->CreateEntity({
                  EntityProperty::EntityType<PlayerEntity>(),
                  { "position",connectedEvent->position.has_value() ? connectedEvent->position.value() : position },
                  { "dimensions", { 30, 30 } },
                    }));

                player->GetComponent<NetComponent>()->ownerClientId = connectedEvent->id;

                scene->GetCameraFollower()->FollowEntity(player);
                scene->GetCameraFollower()->CenterOn(player->Center());
            }
        }
    }
}

PlayerEntity* InputService::GetPlayerByNetId(int netId)
{
    for(auto player : players)
    {
        if(player->net->netId == netId)
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
            scene->replicationManager->ProcessMessageFromClient(message, response, clientId);
        };
    }
    else
    {
        net->onUpdateResponse = [=](SLNet::BitStream& message)
        {
            scene->replicationManager->UpdateClient(message);
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
        auto mouse = scene->GetEngine()->GetInput()->GetMouse();

        if(mouse->LeftPressed())
        {
            for(auto player : players)
            {
                if(player->Bounds().ContainsPoint(scene->GetCamera()->ScreenToWorld(mouse->MousePosition()))
                    && player->net->ownerClientId == scene->replicationManager->localClientId)
                {
                    activePlayer = player;
                    break;
                }
            }
        }

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

                command.keys = keyBits;
                command.fixedUpdateCount = fixedUpdateCount;
                command.timeRecorded = scene->relativeTime - command.fixedUpdateCount * Scene::PhysicsDeltaTime;
                command.netId = self->net->netId;
                fixedUpdateCount = 0;

                if(mouse->RightPressed())
                {
                    bool attack = false;
                    for (auto player : players)
                    {
                        if (player->Bounds().ContainsPoint(scene->GetCamera()->ScreenToWorld(mouse->MousePosition())))
                        {
                            command.attackTarget = true;
                            command.attackNetId = player->net->netId;
                            attack = true;
                            break;
                        }
                    }

                    if (!attack)
                    {
                        command.moveToTarget = true;
                        command.target = scene->GetCamera()->ScreenToWorld(mouse->MousePosition());
                    }
                }

                scene->replicationManager->AddPlayerCommand(command);
            }
        }

        scene->replicationManager->DoClientUpdate(scene->deltaTime, net);
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