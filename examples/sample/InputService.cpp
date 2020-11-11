
#include "InputService.hpp"

#include <slikenet/BitStream.h>
#include <slikenet/peerinterface.h>

#include "Engine.hpp"
#include "PuckEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Memory/Util.hpp"
#include "Net/NetworkPhysics.hpp"
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
        }

        if(net->IsServer())
        {
            scene->CreateEntity({
                EntityProperty::EntityType<PuckEntity>(),
                { "position", { 1800, 1800} },
                { "dimensions", { 15, 15} },
            });
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

        self->net->netId = joinedServerEvent->selfId;
        self->net->isClientPlayer = true;

        scene->replicationManager.localPlayer = self;

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

        player->net->netId = connectedEvent->id;
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
            auto player = GetPlayerByNetId(clientId);
            if (player != nullptr)
            {
                scene->replicationManager.ProcessMessageFromClient(message, response, player->net, clientId);
            }
        };
    }
    else
    {
        net->onUpdateResponse = [=](SLNet::BitStream& message)
        {
            scene->replicationManager.UpdateClient(message);
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
                command.id = ++self->net->nextCommandSequenceNumber;
                command.keys = keyBits;
                command.fixedUpdateCount = fixedUpdateCount;
                command.timeRecorded = scene->timeSinceStart;
                fixedUpdateCount = 0;

                self->net->commands.Enqueue(command);
            }

            scene->GetService<NetworkPhysics>()->UpdateClientPrediction(self->net);
            scene->replicationManager.DoClientUpdate(scene->deltaTime, net);
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

    renderer->RenderString(FontSettings(ResourceManager::GetResource<SpriteFont>("console-font"_sid), 1),
        status.c_str(),
        Vector2(0, 200) + scene->GetCamera()->TopLeft(),
        -1);
}