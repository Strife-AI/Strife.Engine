#pragma once

#include <vector>
#include <memory>
#include <functional>

#include "Renderer/Camera.hpp"

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

struct PipelineRunner
{
    PipelineRunner(RenderPipelineState& state);

    PipelineRunner& UseCamera(Camera* camera);
    PipelineRunner& UseSceneCamera();
    PipelineRunner& UseUiCamera();

    PipelineRunner& ModifyRendererState(const std::function<void(RendererState& state)>& modifyState);
    PipelineRunner& RunStage(const std::function<void(RenderPipelineState& state)>& executeStage);

    PipelineRunner& RenderEntities();
    PipelineRunner& RenderImgui();
    PipelineRunner& RenderConsole();
    PipelineRunner& RenderDebugPrimitives();

    RenderPipelineState& state;
    Camera uiCamera;
};