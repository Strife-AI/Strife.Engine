#pragma once

#include "GL/Shader.hpp"
#include "glm/mat4x4.hpp"
#include "RenderVertex.hpp"
#include "gsl/span"

struct SpriteEffect : Effect
{
    SpriteEffect(Shader* shader, int batchSize = 3 * 200);

    void Start() override;
    void Flush() override;
    void RenderPolygon(gsl::span<RenderVertex> vertices, Texture* texture);

    ShaderUniform<glm::mat4x4> view;
    ShaderUniform<Texture> spriteTexture;

    Vbo<RenderVertex>* vertexVbo;

    std::unique_ptr<RenderVertex[]> vertices;
    int currentVertexCount = 0;
    int maxVerticesInBatch;
    Texture* currentTexture = nullptr;
};