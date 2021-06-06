#include <Resource/ResourceManager.hpp>
#include "IGame.hpp"
#include "Engine.hpp"
#include "SceneManager.hpp"
#include "System/BinaryStreamReader.hpp"
#include "Tools/Console.hpp"
#include "Renderer/SdlManager.hpp"

void IGame::Run()
{
    // Configuration
    {
        EngineConfig config;
        ConfigureEngine(config);
        _engine = std::make_unique<Engine>(config);

        ConfigureGame(_config);

        if (_config.userConfigFile.has_value())
        {
            _engine->GetConsole()->RunUserConfig(_config.userConfigFile.value().c_str());
        }

        LoadResources(ResourceManager::GetInstance());

        _engine->SetGame(this);

        _engine->SetLoadResources([=]
        {

        });

        if (_config.devConsoleFont.has_value())
        {
            auto font = GetResource<SpriteFontResource>(StringId(_config.devConsoleFont->c_str()));
            _engine->GetConsole()->SetFont(FontSettings(font, 0.75));
        }

        if (_config.windowCaption.has_value())
        {
            _engine->GetSdlManager()->SetWindowCaption(_config.windowCaption.value().c_str());
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

void IGame::Render(PipelineRunner& renderPipeline)
{
    renderPipeline.ModifyRendererState([](RendererState& state)
    {
        state.SetDepthBufferEnabled(true);
        state.ClearBuffers();
    });

    if (!(g_fastUpdate.Value()))
    {
        renderPipeline
            .UseSceneCamera()
            .RenderEntities()
            .RenderDebugPrimitives()
            .RenderImgui();
    }

    renderPipeline
        .UseUiCamera()
        .RenderConsole();
}
