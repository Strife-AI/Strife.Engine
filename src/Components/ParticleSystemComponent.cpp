#include <Resource/SpriteResource.hpp>
#include "Resource/ShaderResource.hpp"
#include "ParticleSystemComponent.hpp"
#include "Renderer/Texture.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Engine.hpp"
#include "Resource/ResourceManager.hpp"

static void CreateRectangle(const Rectangle& bounds, const Rectangle& uv, float z, gsl::span<RenderVertex, 6> outVertices)
{
    outVertices[0].position = Vector3(bounds.TopLeft(), z);
    outVertices[0].textureCoord = uv.TopLeft();
    outVertices[0].color = Color::White().ToVector4();

    outVertices[1].position = Vector3(bounds.TopRight(), z);
    outVertices[1].textureCoord = uv.TopRight();
    outVertices[1].color = Color::White().ToVector4();

    outVertices[2].position = Vector3(bounds.BottomLeft(), z);
    outVertices[2].textureCoord = uv.BottomLeft();
    outVertices[2].color = Color::White().ToVector4();

    outVertices[3] = outVertices[2];
    outVertices[3].color = Color::White().ToVector4();

    outVertices[4] = outVertices[1];
    outVertices[4].color = Color::White().ToVector4();

    outVertices[5].position = Vector3(bounds.BottomRight(), z);
    outVertices[5].textureCoord = uv.BottomRight();
    outVertices[5].color = Color::White().ToVector4();
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

    RenderVertex geometry[6];
    CreateRectangle(Rectangle(-sprite.Bounds().Size() / 2, sprite.Bounds().Size()),
        sprite.UVBounds() / sprite.GetTexture()->Size(),
        0,
        geometry);

    particleGeometryVbo->BufferSub(geometry);
}

void ParticleEffect::Start()
{
    glUniformMatrix4fv(glGetUniformLocation(ProgramId(), "view"), 1, GL_FALSE, &camera->ViewMatrix()[0][0]);
    glBindVertexArray(vao);
    shader->Use();
    glUniform1i(glGetUniformLocation(ProgramId(), "spriteTexture"), 0);
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprite.GetTexture()->Id());
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particleBatchSize);
    particleBatchSize = 0;
}

void ParticleSystemComponent::OnAdded()
{
    effect.emplace(GetResource<SpriteResource>("castle")->sprite, 100);
    effect.value().camera = owner->scene->GetCamera();

    IRenderable::effect = &effect.value();

    layer = 0;
    owner->GetEngine()->GetNewRenderer()->AddRenderable(this);
}

void ParticleSystemComponent::Render()
{
    effect.value().DrawParticle(Vector3(owner->Center(), -1));
}
