#include <SDL2/SDL.h>

#include "Engine.hpp"
#include "InputService.hpp"
#include "PlayerEntity.hpp"
#include "Net/NetworkPhysics.hpp"
#include "Scene/IGame.hpp"
#include "Scene/Scene.hpp"
#include "Tools/Console.hpp"

ConsoleVar<int> g_serverPort("port", 6666);

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

    void ConfigureEngine(EngineConfig& config) override
    {
        config.initialConsoleCmd = initialConsoleCmd;
    }

    void BuildScene(Scene* scene) override
    {
        if (scene->MapSegmentName() != "empty-map"_sid)
        {
            scene->AddService<InputService>();
            scene->AddService<NetworkPhysics>(scene->isServer);

            scene->GetEngine()->GetConsole()->Execute("light 0");
        }
    }

    void OnGameStart() override
    {
        GetEngine()->StartLocalServer(g_serverPort.Value(), "isengard"_sid);

        auto engine = GetEngine();
        auto neuralNetworkManager = engine->GetNeuralNetworkManager();

        auto playerDecider = neuralNetworkManager->CreateDecider<PlayerDecider>();
        auto playerTrainer = neuralNetworkManager->CreateTrainer<PlayerTrainer>();

        neuralNetworkManager->CreateNetwork("nn", playerDecider, playerTrainer);
    }

    std::string initialConsoleCmd;
};

int main(int argc, char* argv[])
{
    BreakoutGame game;

    if (argc >= 2)
    {
        game.initialConsoleCmd = argv[1];
    }

    game.Run();

    return 0;
}
