#include "NewRenderer.hpp"
#include "Resource/ResourceManager.hpp"
#include "Renderer/Texture.hpp"
#include "Resource/ShaderResource.hpp"

void LayeredRenderStage::Execute()
{
    Effect* lastEffect = nullptr;
    for (auto layer : layers)
    {
        for (auto& effectPair : layer->renderables)
        {
            Effect* effect = effectPair.first;
            if (effect != lastEffect)
            {
                lastEffect->Stop();
                lastEffect->Flush();
                effect->Start();
                lastEffect = effect;
            }

            for (IRenderable* renderable : effectPair.second)
            {
                renderable->Render();
            }
        }
    }

    if (lastEffect != nullptr)
    {
        lastEffect->Flush();
        lastEffect->Stop();
    }
}

static void CreateRectangle(const Rectangle& bounds, const Rectangle &uv, float z, gsl::span<RenderVertex, 6> outVertices)
{
    outVertices[0].position = Vector3(bounds.TopLeft(), z);
    outVertices[0].textureCoord = uv.TopLeft();

    outVertices[1].position = Vector3(bounds.TopRight(), z);
    outVertices[1].textureCoord = uv.TopRight();

    outVertices[2].position = Vector3(bounds.BottomLeft(), z);
    outVertices[2].textureCoord = uv.BottomLeft();

    outVertices[3] = outVertices[2];
    outVertices[4] = outVertices[1];

    outVertices[5].position = Vector3(bounds.BottomRight(), z);
    outVertices[5].textureCoord = uv.BottomRight();
}

ParticleEffect::ParticleEffect(Sprite& sprite, int maxParticlesPerBatch)
    : Effect(&GetResource<ShaderResource>("particle-shader")->Get()),
    sprite(sprite),
    maxParticlesPerBatch(maxParticlesPerBatch)
{
    particlesVbo = CreateBuffer<ParticleInstance>(maxParticlesPerBatch, VboType::Vertex);
    BindVertexAttribute("offset", particlesVbo, [=](auto p) { return &p->position; });

    particleGeometryVbo = CreateBuffer<RenderVertex>(6, VboType::Vertex);

    RenderVertex geometry[6];
    CreateRectangle(Rectangle(-sprite.Bounds().Size() / 2, sprite.Bounds().Size()),
        sprite.UVBounds(),
        -1,
        geometry);

    particleGeometryVbo->BufferSub(geometry);
}

void ParticleEffect::Start()
{
    glUniform1i(glGetUniformLocation(ProgramId(), "spriteTexture"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprite.GetTexture()->Id());
    shader->Use();
    // TODO bind vao
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
    // TODO: draw instanced
}
