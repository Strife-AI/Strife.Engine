#include "RenderPipeline.hpp"
#include "Renderer.hpp"
#include "Scene/Scene.hpp"
#include "Tools/Console.hpp"
#include "Scene/IEntityEvent.hpp"

PipelineRunner& PipelineRunner::ModifyRendererState(const std::function<void(RendererState&)>& modifyState)
{
    modifyState(*state.rendererState);
    return *this;
}

PipelineRunner& PipelineRunner::RunStage(const std::function<void(RenderPipelineState&)>& executeStage)
{
    executeStage(state);
    state.rendererState->FlushActiveEffect();
    return *this;
}

PipelineRunner& PipelineRunner::RenderEntities()
{
    return RunStage([&](RenderPipelineState& state) { state.scene->RenderEntities(state.renderer); });
}

PipelineRunner& PipelineRunner::RenderImgui()
{
    state.scene->SendEvent(RenderImguiEvent());
    return *this;
}

PipelineRunner& PipelineRunner::RenderConsole()
{
    return RunStage([&](RenderPipelineState& state)
    {
        auto console = state.scene->GetEngine()->GetConsole();
        if (console->IsOpen())
        {
            console->Render(state.renderer, state.deltaTime);
        }
    });
}

PipelineRunner& PipelineRunner::UseCamera(Camera* camera)
{
    state.renderer->SetCamera(camera);
    return *this;
}

PipelineRunner& PipelineRunner::RenderDebugPrimitives()
{
    state.renderer->RenderDebugPrimitives();
    return *this;
}

PipelineRunner& PipelineRunner::UseSceneCamera()
{
    return UseCamera(state.scene->GetCamera());
}

PipelineRunner::PipelineRunner(RenderPipelineState& state)
    : state(state)
{
    auto screenSize = state.scene->GetCamera()->ScreenSize();
    uiCamera.SetScreenSize(screenSize);
    uiCamera.SetPosition(screenSize / 2);
}

PipelineRunner& PipelineRunner::UseUiCamera()
{
    UseCamera(&uiCamera);
    return *this;
}
