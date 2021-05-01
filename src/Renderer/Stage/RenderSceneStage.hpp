#pragma once

#include "RenderPipeline.hpp"

struct RenderSceneStage : IRenderStage
{
    void Execute(const RenderPipelineState& state) override;
};

