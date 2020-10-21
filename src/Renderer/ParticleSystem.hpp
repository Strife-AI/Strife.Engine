#pragma once

#include "Math/Rectangle.hpp"
#include "Math/Vector2.hpp"
#include "Scene/Scene.hpp"
#include "Memory/ResourcePool.hpp"

struct ZeroGravityParticle
{
    Vector2 position;
    Vector2 velocity;
    float creationTime;
    Color color;
};

struct ParticleGroup
{
    static constexpr int MaxParticles = 8192;
    FixedSizeVector<ZeroGravityParticle, MaxParticles> particles;
};

enum class ParticleExpiration
{
    Height,
    Time
};

enum class ParticleShape
{
    Square,
    Circle
};

struct ParticleSystem
{
    ParticleSystem();
    ~ParticleSystem();

    void CreateParticle(Vector2 position, Vector2 velocity);

    void Render(Renderer* renderer, float deltaTime);
    void Update(float deltaTime);

    Scene* scene;
    float minY;
    float particleSize = 2;
    Vector2 acceleration;
    float spawnRatePerSecond = 1;
    float lastSpawnTime;
    float expirationTime = 0;
    Rectangle spawnRegion;
    ResourceHandle<ParticleGroup> particles;
    ParticleExpiration expirationType = ParticleExpiration::Height;
    Color spawnColor = Color(128, 128, 128);
    Vector2 launchVelocity;
    ParticleShape shape = ParticleShape::Square;
    bool allowCulling = true;
    float colorChange = 0;
};