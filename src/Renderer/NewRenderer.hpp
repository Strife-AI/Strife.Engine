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

struct ParticleInstance
{
    Vector3 position;
};

struct ParticleEffect : Effect
{
    ParticleEffect(Sprite& sprite, int maxParticlesPerBatch);

    void Start() override;

    void DrawParticle(Vector3 position);

    void Flush() override;

    Sprite sprite;
    Vbo<ParticleInstance>* particlesVbo;
    Vbo<RenderVertex>* particleGeometryVbo;

    int maxParticlesPerBatch;
    std::unique_ptr<ParticleInstance[]> particleBatch;
    int particleBatchSize = 0;
};

struct ParticleSystem : IRenderable
{
    ParticleSystem()
    {
        //effect = particleEffect = new ParticleEffect;
    }

    void Render() override
    {

    }

    ParticleEffect* particleEffect;
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
        AddRenderable(new ParticleSystem);
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
        layers[renderable->layer]->renderables[renderable->effect].push_back(renderable);
    }

    std::vector<RenderStage*> stages;
    std::map<int, RenderLayer*> layers;
};