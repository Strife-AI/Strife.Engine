#pragma once

#include "RenderPipeline.hpp"

struct ClearBuffersStage : IRenderStage
{
    void Execute(const RenderPipelineState& state) override;

    bool clearDepthBuffer = true;
    bool clearColorBuffer = true;
};