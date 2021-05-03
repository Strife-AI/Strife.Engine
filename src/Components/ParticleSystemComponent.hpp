#pragma once

#include "Scene/EntityComponent.hpp"
#include "Renderer/RenderVertex.hpp"

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

    ShaderUniform<Texture> spriteTexture;
};

struct Particle
{
    Vector3 position;
    Vector3 velocity;
    float spawnTime;
};

DEFINE_COMPONENT(ParticleSystemComponent)
{
    void OnAdded() override;
    void Render(Renderer* renderer) override;
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