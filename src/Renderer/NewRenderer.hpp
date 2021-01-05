#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include <Math/Vector3.hpp>
#include <memory>

#include "Math/Vector2.hpp"
#include "GL/Shader.hpp"
#include "Sprite.hpp"
#include "RenderVertex.hpp"

struct IRenderable
{
    virtual void Render() = 0;

    Effect* effect;
    int layer;
};

struct RenderLayer
{
    std::unordered_map<Effect*, std::vector<IRenderable*>> renderables;
};

struct RenderStage
{
    virtual void Execute() = 0;
};

struct LayeredRenderStage : RenderStage
{
    void Execute() override;

    std::vector<RenderLayer*> layers;
};

class NewRenderer
{
public:
    NewRenderer()
    {

    }

    void Render()
    {
       for (auto stage : stages)
       {
           stage->Execute();
       }
    }

    void AddRenderable(IRenderable* renderable)
    {
        // TODO: use unique pointer or something for managing layers
        auto layer = GetLayer(renderable->layer);
        layer->renderables[renderable->effect].push_back(renderable);
    }

    RenderLayer* GetLayer(int id)
    {
        auto& layer = layers[id];
        if (layer == nullptr)
        {
            layer = new RenderLayer;
        }

        return layer;
    }

    std::vector<RenderStage*> stages;
    std::map<int, RenderLayer*> layers;
};