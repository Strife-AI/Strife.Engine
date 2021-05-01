#include "RenderPipeline.hpp"
#include "Renderer.hpp"

void RenderPipeline::Execute(const RenderPipelineState& state)
{
    state.renderer->BeginRender(state.scene, state.camera, Vector2(0, 0), state.deltaTime, state.absoluteTime);

    for (auto& stage : stages)
    {
        stage->Run();
    }
}
