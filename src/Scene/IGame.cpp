#include "IGame.hpp"
#include "Engine.hpp"
#include "SceneManager.hpp"
#include "System/BinaryStreamReader.hpp"
#include "Tools/Console.hpp"

void IGame::Run()
{
    // Configuration
    {
        EngineConfig config;
        ConfigureEngine(config);
        _engine = std::make_unique<Engine>(config);
        _engine->SetGame(this);

        ConfigureGame(_config);

        _engine->SetLoadResources([=]
        {
            for(auto& resourcePack : _config.resourcePacks)
            {
                BinaryStreamReader reader;
                reader.Open(resourcePack.c_str());
                ResourceManager::LoadResourceFile(reader);
            }
        });

        if (_config.userConfigFile.has_value())
        {
            _engine->GetConsole()->RunUserConfig(_config.userConfigFile.value().c_str());
        }

        if(_config.devConsoleFont.has_value())
        {
            auto font = ResourceManager::GetResource<SpriteFont>(StringId(_config.devConsoleFont->c_str()));
            _engine->GetConsole()->SetFont(FontSettings(font, 1));
        }
    }

    // Run the game
    {
        OnGameStart();

        while (_engine->ActiveGame())
        {
            _engine->RunFrame();
        }
    }
}
