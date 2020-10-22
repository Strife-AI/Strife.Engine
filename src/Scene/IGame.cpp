#include "IGame.hpp"
#include "Engine.hpp"
#include "SceneManager.hpp"
#include "Tools/Console.hpp"

void IGame::Run()
{
    // Configuration
    {
        EngineConfig config;
        ConfigureEngine(config);
        _engine = Engine::Initialize(config);
        _engine->SetGame(this);

        ConfigureGame(_config);

        if (_config.userConfigFile.has_value())
        {
            _engine->GetConsole()->RunUserConfig(_config.userConfigFile.value().c_str());
        }

        _engine->GetSceneManager()->TrySwitchScene(_config.defaultScene);
    }

    // Run the game
    {
        while (_engine->ActiveGame())
        {
            _engine->RunFrame();
        }
    }

    // FIXME
    _engine->~Engine();
}
