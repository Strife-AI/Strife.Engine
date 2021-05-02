#include "RenderPipeline.hpp"
#include "Renderer.hpp"

void RenderPipeline::Execute(const RenderPipelineState& state)
{
    for (auto& stage : stages)
    {
        stage->Run(state);
    }
}
