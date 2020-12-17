
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
#include "MessageHud.hpp"

#include "CastleEntity.hpp"

InputButton g_quit = InputButton(SDL_SCANCODE_ESCAPE);
InputButton g_upButton(SDL_SCANCODE_W);
InputButton g_downButton(SDL_SCANCODE_S);
InputButton g_leftButton(SDL_SCANCODE_A);
InputButton g_rightButton(SDL_SCANCODE_D);
InputButton g_nextPlayer(SDL_SCANCODE_TAB);

ConsoleVar<bool> autoConnect("auto-connect", false);

void InputService::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<SceneLoadedEvent>())
    {
        if (!scene->isServer && autoConnect.Value())
        {
            scene->GetEngine()->GetConsole()->Execute("connect 127.0.0.1");

            //scene->GetCameraFollower()->FollowMouse();
            //scene->GetCameraFollower()->CenterOn({ 800, 800});
        }

        if (scene->isServer)
        {
            EntityProperty properties[] = {
                    EntityProperty::EntityType<PuckEntity>(),
                    { "position", { 1800, 1800} },
                    { "dimensions", { 32, 32} },
            };

            scene->CreateEntity(EntityDictionary(properties));

            for (auto spawn : scene->GetEntitiesOfType<CastleEntity>())
            {
                spawns.push_back(spawn);
            }
        }
    }
    if (ev.Is<UpdateEvent>())
    {
        if (scene->isServer && !gameOver)
        {
            bool multipleClientsConnected = scene->replicationManager->GetClients().size() >= 2;

            if (multipleClientsConnected)
            {
                if (players.size() == 0)
                {
                    scene->SendEvent(BroadcastToClientMessage("Draw!"));
                    gameOver = true;
                }
                else
                {
                    bool onePlayerLeft = true;
                    for (int i = 1; i < players.size(); ++i)
                    {
                        if (players[i]->net->ownerClientId != players[0]->net->ownerClientId)
                        {
                            onePlayerLeft = false;
                            break;
                        }
                    }

                    if (onePlayerLeft)
                    {
                        gameOver = true;
                        auto& name = scene->replicationManager->GetClient(players[0]->net->ownerClientId).clientName;
                        scene->SendEvent(BroadcastToClientMessage(name + " wins!"));
                    }
                }
            }
        }

        HandleInput();
    }
    else if (auto renderEvent = ev.Is<RenderEvent>())
    {
        if (scene->isServer)
        {
            std::string result;

            for(auto& c : scene->replicationManager->GetClients())
            {
                result += "[" + std::to_string(c.first) + "] = " + std::to_string((int)scene->replicationManager->GetCurrentSnapshotId() - (int)c.second.lastReceivedSnapshotId) + "\n";
            }

            status.VFormat("%s", result.c_str());
        }

        Render(renderEvent->renderer);
    }
    else if (auto fixedUpdateEvent = ev.Is<FixedUpdateEvent>())
    {
        if (scene->isServer)
        {

        }
        else
        {
            ++fixedUpdateCount;
        }

        ++currentFixedUpdateId;
    }
    else if (auto joinedServerEvent = ev.Is<JoinedServerEvent>())
    {
        scene->replicationManager->localClientId = joinedServerEvent->selfId;
    }
    else if (auto connectedEvent = ev.Is<PlayerConnectedEvent>())
    {
        if (scene->isServer)
        {
            // Assign spawn
            for (auto spawn : spawns)
            {
                if (spawn->net->ownerClientId < 0)
                {
                    spawn->net->ownerClientId = connectedEvent->id;

                    for (int i = 0; i < 4; ++i)
                    {
                        spawn->SpawnPlayer();
                    }

                    break;
                }
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
    if(scene->isServer)
    {
        scene->GetCameraFollower()->FollowMouse();

        if (scene->GetEngine()->GetServerGame())
        {
            scene->GetEngine()->GetServerGame()->onUpdateRequest = [=](SLNet::BitStream& message, SLNet::BitStream& response, int clientId)
            {
                scene->replicationManager->Server_ProcessUpdateRequest(message, response, clientId);
            };
        }
    }
    else
    {
        if (scene->GetEngine()->GetClientGame())
        {
            scene->GetEngine()->GetClientGame()->onUpdateResponse = [=](SLNet::BitStream& message)
            {
                auto& c = scene->replicationManager->GetClients()[scene->replicationManager->localClientId].commands;

                int count = 0;
                for(auto it : c)
                {
                    ++count;
                }

                status.VFormat("Commands behind: %d\n", count);

                auto last = c.IsEmpty() ? 0.0f : (*(--c.end())).timeRecorded;

                //status.VFormat("%d bytes (%f ms)", NearestPowerOf2(message.GetNumberOfUnreadBits(), 8) / 8, (scene->relativeTime - last) * 1000);

                scene->replicationManager->Client_ReceiveUpdateResponse(message);
            };
        }
    }
}

void InputService::HandleInput()
{
    if (g_quit.IsPressed())
    {
        scene->GetEngine()->QuitGame();
    }

    if (!scene->isServer)
    {
        if(scene->deltaTime == 0)
        {
            return;
        }

        auto mouse = scene->GetEngine()->GetInput()->GetMouse();

        if(mouse->LeftPressed())
        {
            for(auto player : players)
            {
                if(player->Bounds().ContainsPoint(scene->GetCamera()->ScreenToWorld(mouse->MousePosition()))
                    && player->net->ownerClientId == scene->replicationManager->localClientId)
                {
                    PlayerEntity* oldPlayer;
                    if(activePlayer.TryGetValue(oldPlayer))
                    {
                        oldPlayer->GetComponent<PlayerEntity::NeuralNetwork>()->mode = NeuralNetworkMode::Deciding;
                    }

                    activePlayer = player;
                    player->GetComponent<PlayerEntity::NeuralNetwork>()->mode = NeuralNetworkMode::CollectingSamples;

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
                command.netId = self->net->netId;
                fixedUpdateCount = 0;

                if(mouse->RightPressed())
                {
                    bool attack = false;
                    for (auto entity : scene->GetEntities())
                    {
                        auto netComponent = entity->GetComponent<NetComponent>(false);

                        if(netComponent == nullptr)
                        {
                            continue;
                        }

                        if(entity->GetComponent<HealthBarComponent>(false) == nullptr)
                        {
                            continue;
                        }

                        if (entity->Bounds().ContainsPoint(scene->GetCamera()->ScreenToWorld(mouse->MousePosition())))
                        {
                            command.attackTarget = true;
                            command.attackNetId = netComponent->netId;
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

                scene->replicationManager->Client_AddPlayerCommand(command);
            }
        }

        scene->replicationManager->Client_SendUpdateRequest(scene->deltaTime, scene->GetEngine()->GetClientGame());
    }
}

void InputService::Render(Renderer* renderer)
{
    PlayerEntity* currentPlayer;
    if (activePlayer.TryGetValue(currentPlayer))
    {
        renderer->RenderRectangleOutline(currentPlayer->Bounds(), Color::White(), -1);
    }

#if false
    renderer->RenderString(FontSettings(ResourceManager::GetResource<SpriteFont>("console-font"_sid), 1),
        status.c_str(),
        Vector2(0, 200) + scene->GetCamera()->TopLeft(),
        -1);
#endif
}