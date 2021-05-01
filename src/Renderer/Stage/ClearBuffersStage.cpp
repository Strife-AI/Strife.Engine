#include "ClearBuffersStage.hpp"
#include "GL/gl3w.h"

void ClearBuffersStage::Execute(const RenderPipelineState& state)
{
    uint32_t flags = 0;
    if (clearColorBuffer) flags |= GL_COLOR_BUFFER_BIT;
    if (clearDepthBuffer) flags |= GL_DEPTH_BUFFER_BIT;

    glClear(flags);
}
