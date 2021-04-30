#include "SpriteEffect.hpp"
#include "Texture.hpp"
#include "Camera.hpp"

SpriteEffect::SpriteEffect(Shader* shader, int batchSize)
    : Effect(shader)
{
    vertexVbo = CreateBuffer<RenderVertex>(batchSize, VboType::Vertex);
    maxVerticesInBatch = batchSize;
    vertices = std::make_unique<RenderVertex[]>(maxVerticesInBatch);

    BindVertexAttribute("aPos", vertexVbo, [=](auto rv) { return &rv->position; });
    BindVertexAttribute("textureCoord", vertexVbo, [=](auto rv) { return &rv->textureCoord; });
    BindVertexAttribute("color", vertexVbo, [=](auto rv) { return &rv->color; });

    view = GetUniform<glm::mat4x4>("view");
    spriteTexture = GetUniform<Texture>("spriteTexture");
}

void SpriteEffect::Start()
{
    glUniformMatrix4fv(view.id, 1, GL_FALSE, &renderer->camera->ViewMatrix()[0][0]);
}

void SpriteEffect::RenderPolygon(gsl::span<RenderVertex> vertices, Texture* texture)
{
    if (currentTexture == nullptr)
    {
        currentTexture = texture;
    }

    if (texture->Id() != currentTexture->Id())
    {
        FlushEffect();
    }

    // Split the convex polygon into triangles
    {
        int first = 0;

        for (int i = 1; i < vertices.size() - 1; ++i)
        {
            int a = first, b = first + i, c = first + i + 1;

            if (currentVertexCount + 3 > maxVerticesInBatch)
            {
                FlushEffect();
            }

            this->vertices[currentVertexCount++] = vertices[a];
            this->vertices[currentVertexCount++] = vertices[b];
            this->vertices[currentVertexCount++] = vertices[c];
        }
    }
}

void SpriteEffect::Flush()
{
    if (currentVertexCount == 0) return;

    renderer->BindTexture(spriteTexture, currentTexture, 0);
    vertexVbo->BufferSub(gsl::span<RenderVertex>(vertices.get(), currentVertexCount));
    glDrawArrays(GL_TRIANGLES, 0, currentVertexCount);

    currentVertexCount = 0;
}
