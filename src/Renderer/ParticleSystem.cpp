#include "ParticleSystem.hpp"

#include <random>



#include "Renderer.hpp"
//#include "Entity/RenderLayers.hpp"
#include "Math/Random.hpp"

static ResourcePool<ParticleGroup> g_particlePool;

ParticleSystem::ParticleSystem()
{

}

ParticleSystem::~ParticleSystem()
{
    particles.Release();
}

void ParticleSystem::CreateParticle(Vector2 position, Vector2 velocity)
{
    if (particles.Value() != nullptr)
    {
        ZeroGravityParticle particle;
        particle.velocity = velocity;
        particle.position = position;
        particle.creationTime = scene->timeSinceStart;
        particle.color = spawnColor;

        particles->particles.PushBackIfRoom(particle);
    }
}

void ParticleSystem::Render(Renderer* renderer, float deltaTime)
{
    Vector2 expandedSize(2000, 2000);

    Rectangle approximateBounds(
        spawnRegion.TopLeft() - expandedSize / 2,
        spawnRegion.Size() + expandedSize);

    auto bounds = renderer->GetCamera()->Bounds();
    bounds.SetPosition(renderer->RenderOffset());

    if(allowCulling && !approximateBounds.IntersectsWith(bounds))
    {
        if (particles.Value() != nullptr)
        {
            particles.Release();
        }

        return;
    }

    if(particles.Value() == nullptr)
    {
        lastSpawnTime = scene->timeSinceStart;
        particles = g_particlePool.Borrow();
    }

    int lastAlive = 0;
    Vector2 size(particleSize, particleSize);

    for (int i = 0; i < particles->particles.Size(); ++i)
    {
        auto& particle = particles->particles[i];
        bool alive = expirationType == ParticleExpiration::Height && particle.position.y > minY
            || expirationType == ParticleExpiration::Time && scene->timeSinceStart - particle.creationTime < expirationTime;

        if (alive)
        {
            particle.velocity += acceleration * deltaTime;
            particle.position += particle.velocity * deltaTime;

            float transparency = 0;

            if (expirationType == ParticleExpiration::Height) transparency = Clamp((particle.position.y - minY) / 40, 0.0f, 1.0f);
            else transparency = Clamp(1 - (scene->timeSinceStart - particle.creationTime) / expirationTime, 0.0f, 1.0f);
            
            if (shape == ParticleShape::Square)
            {
                renderer->RenderRectangle(
                    Rectangle(particle.position - size / 2, size).AddPosition(-renderer->GetCamera()->TopLeft()),
                    Color(particle.color.r, particle.color.g, particle.color.b, transparency * 255),
                    0); // FIXME MW ParticlesDetailsRenderLayer);
            }
            else
            {
                renderer->RenderCircle(
                    particle.position,
                    particleSize,
                    Color(particle.color.r, particle.color.g, particle.color.b, transparency * 255) * (1 - (1 - transparency) * colorChange),
                    0, // FIXME MW ParticlesDetailsRenderLayer,
                    8);
            }

            particles->particles[lastAlive++] = particle;
        }
    }

    particles->particles.Resize(lastAlive);
}

void ParticleSystem::Update(float deltaTime)
{
    if(particles.Value() == nullptr)
    {
        return;
    }

    if(spawnRatePerSecond == 0)
    {
        return;
    }

    while (scene->timeSinceStart - lastSpawnTime > 1.0 / spawnRatePerSecond)
    {
        Vector2 position = Rand(
            spawnRegion.TopLeft() + Vector2(particleSize / 2),
            spawnRegion.BottomRight() - Vector2(particleSize / 2));

        CreateParticle(position, launchVelocity);

        lastSpawnTime += 1.0f / spawnRatePerSecond;
    }
}