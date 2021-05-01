#pragma once

#include <vector>
#include <memory>

struct Camera;
struct Renderer;
struct RendererState;
struct Scene;

struct RenderPipelineState
{
    Camera* camera;
    Renderer* renderer;
    RendererState* rendererState;
    Scene* scene;
    float deltaTime;
    float absoluteTime;
};

struct IRenderStage
{
    void Run(const RenderPipelineState& state)
    {
        Execute(state);
    }

protected:
    virtual void Execute(const RenderPipelineState& state) { }
};


struct RenderPipeline
{
    void Execute(const RenderPipelineState& state);

    std::vector<std::unique_ptr<IRenderStage>> stages;
};