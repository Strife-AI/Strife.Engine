#include "RenderSceneStage.hpp"
#include "Scene/Scene.hpp"

void RenderSceneStage::Execute(const RenderPipelineState& state)
{
    state.scene->RenderEntities(state.renderer);
}
