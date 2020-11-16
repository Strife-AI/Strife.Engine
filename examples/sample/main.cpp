#include <SDL2/SDL.h>


#include "Engine.hpp"
#include "InputService.hpp"
#include "PlayerEntity.hpp"
#include "Scene/IGame.hpp"
#include "Scene/Scene.hpp"
#include "Tools/Console.hpp"

struct BreakoutGame : IGame
{
    void ConfigureGame(GameConfig& config) override
    {
        config
            .SetDefaultScene("isengard"_sid)
            .SetWindowCaption("Breakout")
            .SetGameName("breakout")
            .ExecuteUserConfig("user.cfg")
            .EnableDevConsole("console-font")
            .AddResourcePack("sample.x2rp");
    }

    void BuildScene(Scene* scene) override
    {
        if (scene->MapSegmentName() != "empty-map"_sid)
        {
            scene->AddService<InputService>();
            //FIXME MW scene->GetEngine()->GetConsole()->Execute("light 0");
        }
    }
};

int main(int argc, char* argv[])
{
    BreakoutGame game;
    game.Run();

    return 0;
}
