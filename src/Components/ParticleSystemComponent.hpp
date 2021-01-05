#pragma once

#include "Scene/EntityComponent.hpp"
#include "Renderer/NewRenderer.hpp"

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

    struct Camera* camera;
};

DEFINE_COMPONENT(ParticleSystemComponent), IRenderable
{
    void OnAdded() override;
    void Render() override;

    std::optional<ParticleEffect> effect;
};