#include <Resource/SpriteResource.hpp>
#include "Resource/ShaderResource.hpp"
#include "ParticleSystemComponent.hpp"
#include "Renderer/Texture.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Engine.hpp"
#include "Resource/ResourceManager.hpp"
#include "Math/Random.hpp"

static void CreateRectangle(const Rectangle& bounds, const Rectangle& uv, float z, gsl::span<RenderVertex, 6> outVertices, Color color)
{
    outVertices[0].position = Vector3(bounds.TopLeft(), z);
    outVertices[0].textureCoord = uv.TopLeft();
    outVertices[0].color = color.ToVector4();

    outVertices[1].position = Vector3(bounds.TopRight(), z);
    outVertices[1].textureCoord = uv.TopRight();
    outVertices[1].color = color.ToVector4();

    outVertices[2].position = Vector3(bounds.BottomLeft(), z);
    outVertices[2].textureCoord = uv.BottomLeft();
    outVertices[2].color = color.ToVector4();

    outVertices[3] = outVertices[2];
    outVertices[3].color = color.ToVector4();

    outVertices[4] = outVertices[1];
    outVertices[4].color = color.ToVector4();

    outVertices[5].position = Vector3(bounds.BottomRight(), z);
    outVertices[5].textureCoord = uv.BottomRight();
    outVertices[5].color = color.ToVector4();
}

ParticleEffect::ParticleEffect(Sprite& sprite, int maxParticlesPerBatch)
    : Effect(&GetResource<ShaderResource>("particle-shader")->Get()),
      sprite(sprite),
      maxParticlesPerBatch(maxParticlesPerBatch),
      particleBatch(std::make_unique<ParticleInstance[]>(maxParticlesPerBatch))
{
    particlesVbo = CreateBuffer<ParticleInstance>(maxParticlesPerBatch, VboType::Vertex);
    BindVertexAttribute("offset", particlesVbo, [=](auto p) { return &p->position; }, 1);

    particleGeometryVbo = CreateBuffer<RenderVertex>(6, VboType::Vertex);

    BindVertexAttribute("position", particleGeometryVbo, [=](auto p) { return &p->position; });
    BindVertexAttribute("color", particleGeometryVbo, [=](auto p) { return &p->color; });
    BindVertexAttribute("textureCoord", particleGeometryVbo, [=](auto p) { return &p->textureCoord; });

    int r = 10;

    RenderVertex geometry[6];
    CreateRectangle(Rectangle(-sprite.Bounds().Size() / 2, sprite.Bounds().Size()),
        sprite.UVBounds(),
        -1,
        geometry,
        Color(r, r, r, 200));

    particleGeometryVbo->BufferSub(geometry);

    spriteTexture = GetUniform<Texture>("spriteTexture");
}

void ParticleEffect::Start()
{
    glUniformMatrix4fv(glGetUniformLocation(ProgramId(), "view"), 1, GL_FALSE, &camera->ViewMatrix()[0][0]);
    glBindVertexArray(vao);
    shader->Use();
    renderer->BindTexture(spriteTexture, sprite.GetTexture(), 0);
}

void ParticleEffect::DrawParticle(Vector3 position)
{
    if (particleBatchSize == maxParticlesPerBatch)
    {
        Flush();
    }

    particleBatch[particleBatchSize].position = position;
    ++particleBatchSize;
}

void ParticleEffect::Flush()
{
    particlesVbo->BufferSub(gsl::span<ParticleInstance>(particleBatch.get(), particleBatchSize));
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particleBatchSize);
    particleBatchSize = 0;
}

void ParticleEffect::Stop()
{
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void ParticleSystemComponent::OnAdded()
{
    effect.emplace(GetResource<SpriteResource>("particle")->sprite, 100);
    effect.value().camera = owner->scene->GetCamera();

    IRenderable::effect = &effect.value();

    layer = 0;
    owner->GetEngine()->GetNewRenderer()->AddRenderable(this);
}

void ParticleSystemComponent::Render()
{
    for (auto& particle : particles)
    {
        effect.value().DrawParticle(particle.position);
    }
}

void ParticleSystemComponent::Update(float deltaTime)
{
    // Remove dead particles
    {
        int aliveCount = 0;
        for (auto& particle : particles)
        {
            if (particle.spawnTime + particleLifetime < GetScene()->relativeTime || true)
            {
                particle.position = Vector3(particle.position.XY() + particle.velocity.XY() * deltaTime, 0);
                particles[aliveCount++] = particle;
            }
        }

        particles.resize(aliveCount);
    }

    _particlesToSpawn += spawnRatePerSecond * deltaTime;

    while (_particlesToSpawn >= 1)
    {
        Particle particle;
        particle.position = Vector3(owner->ScreenCenter() + Rand(relativeSpawnBounds.TopLeft(), relativeSpawnBounds.BottomRight()), 0);

        float angle = spawnAngle + Rand(-spawnAngleRange / 2, spawnAngleRange / 2);
        particle.velocity = Vector3(Vector2(cos(angle), sin(angle)) * Rand(minSpeed, maxSpeed), 0);
        particle.spawnTime = GetScene()->relativeTime;

        particles.push_back(particle);

        _particlesToSpawn -= 1;
    }
}
