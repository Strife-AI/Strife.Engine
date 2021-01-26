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
    void Stop() override;
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

struct Particle
{
    Vector3 position;
    Vector3 velocity;
    float spawnTime;
};

DEFINE_COMPONENT(ParticleSystemComponent), IRenderable
{
    void OnAdded() override;
    void Render() override;
    void Update(float deltaTime) override;

    std::optional<ParticleEffect> effect;
    std::vector<Particle> particles;

    float spawnRatePerSecond;
    Rectangle relativeSpawnBounds;
    float spawnAngle;
    float spawnAngleRange;
    float minSpeed;
    float maxSpeed;
    float particleLifetime;

private:
    float _particlesToSpawn = 0;
};