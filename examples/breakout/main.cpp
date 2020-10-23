#include <SDL2/SDL.h>


#include "Engine.hpp"
#include "Scene/IGame.hpp"
#include "Scene/Scene.hpp"

struct Service : ISceneService
{
    void ReceiveEvent(const IEntityEvent& ev) override
    {
        if(ev.Is<PreUpdateEvent>())
        {
            if(quit.IsPressed())
            {
                scene->GetEngine()->QuitGame();
            }
        }
    }

    InputButton quit = InputButton(SDL_SCANCODE_ESCAPE);
};

struct BreakoutGame : IGame
{
    void ConfigureGame(GameConfig& config) override
    {
        config
            .SetDefaultScene("empty-map"_sid)
            .SetWindowCaption("Breakout")
            .SetGameName("breakout")
            .ExecuteUserConfig("user.cfg")
            .EnableDevConsole("console-font")
            .AddResourcePack("sample.x2rp");
    }

    void BuildScene(Scene* scene) override
    {
        scene->AddService<Service>();
    }
};

int main(int argc, char* argv[])
{
    BreakoutGame game;
    game.Run();

    return 0;
}
