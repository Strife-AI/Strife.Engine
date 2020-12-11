#pragma once


#include <Math/Vector3.hpp>
#include <glm/mat4x4.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/RenderVertex.hpp>
#include <vector>
#include "Shader.hpp"

struct LineRenderer
{
    static constexpr int MaxVerticesPerBatch = 512;
    static constexpr int VertexLocation = 0;
    static constexpr int ColorLocation = 1;

    void Initialize(Shader* shader_);

    void DrawLine(const Vector3& start, const Vector3& end, Color color);
    void Render(const glm::mat4& viewMatrix);

    std::vector<RenderVertex> vertices;
    Shader* shader;
    GLuint voa;
    GLuint vertexVbo;
    GLuint viewMatrixLocation;
};
