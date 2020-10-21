#pragma once

#include "Renderer.hpp"
#include "Math/Rectangle.hpp"
#include "Math/Vector2.hpp"
#include "Memory/FixedSizeVector.hpp"

struct Particle
{
    Vector2 position;
    Vector2 velocity;
};

class ParticleRenderer : public ICustomTransparencyRenderer
{
public:
    void SpawnParticle();
    void SetSpawnRegion(Rectangle spawnRegion);
    void Render(SpriteBatcher* batcher, Camera* camera, float depth) const override;
    void Update(float deltaTime);

    bool canSpawn = true;

private:
    float _particleSpawnTime = 0;
    float _spawnRate = 500;
    FixedSizeVector<Particle, 1024> _particles;
    Rectangle spawnRegion;
};