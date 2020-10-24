#include "InputService.hpp"

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
    else if (auto playerAdded = ev.Is<PlayerAddedToGame>())
    {
        players.push_back(playerAdded->player);
    }
    else if (auto playerRemoved = ev.Is<PlayerRemovedFromGame>())
    {
        RemoveFromVector(players, playerRemoved->player);
    }
    else if(ev.Is<SceneLoadedEvent>())
    {
        SwitchControlledPlayer(players[0]);
    }
}

void InputService::HandleInput()
{
    if (g_quit.IsPressed())
    {
        scene->GetEngine()->QuitGame();
    }

    PlayerEntity* player;
    if (activePlayer.TryGetValue(player))
    {
        if (g_nextPlayer.IsPressed())
        {
            for (int i = 0; i < players.size(); ++i)
            {
                if (players[i] == player)
                {
                    SwitchControlledPlayer(players[(i + 1) % players.size()]);
                }
            }
        }

        auto moveDirection = Vector2(
            g_leftButton.IsDown() ? -1 : g_rightButton.IsDown() ? 1 : 0,
            g_upButton.IsDown() ? -1 : g_downButton.IsDown() ? 1 : 0);

        float moveSpeed = 300;

        player->SetMoveDirection(moveDirection * moveSpeed);
    }
    else
    {
        if (g_nextPlayer.IsPressed())
        {
            auto closestPlayer = FindClosestPlayerOrNull(scene->GetCamera()->Position());
            if (closestPlayer != nullptr)
            {
                scene->GetCameraFollower()->FollowEntity(closestPlayer);
            }
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

void InputService::SwitchControlledPlayer(PlayerEntity* player)
{
    PlayerEntity* currentPlayer;
    if(activePlayer.TryGetValue(currentPlayer))
    {
        currentPlayer->SetMoveDirection({ 0, 0 });
    }

    activePlayer = player;

    if(player != nullptr)
    {
        scene->GetCameraFollower()->FollowEntity(player);
    }
}

PlayerEntity* InputService::FindClosestPlayerOrNull(Vector2 point)
{
    if(players.size() == 0)
    {
        return nullptr;
    }

    PlayerEntity* closest = players[0];

    for(int i = 1; i < players.size(); ++i)
    {
        if((players[i]->Center() - point).Length() < (closest->Center() - point).Length())
        {
            closest = players[i];
        }
    }

    return closest;
}
