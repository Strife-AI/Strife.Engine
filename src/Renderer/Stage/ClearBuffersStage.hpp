#pragma once

#include "RenderPipeline.hpp"

struct ClearBuffersStage : IRenderStage
{
    bool clearDepthBuffer = true;
    bool clearColorBuffer = true;

    void Execute(const RenderPipelineState& state) override;
};