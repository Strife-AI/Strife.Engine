#include "LineRenderer.hpp"
#include "gl3w.h"

void LineRenderer::Initialize(Shader* shader_)
{
    shader = shader_;

    glGenVertexArrays(1, &voa);
    glBindVertexArray(voa);

    glGenBuffers(1, &vertexVbo);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RenderVertex) * MaxVerticesPerBatch, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(VertexLocation);

    glVertexAttribPointer(
            VertexLocation,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(RenderVertex),
            0);

    glEnableVertexAttribArray(ColorLocation);

    glVertexAttribPointer(
            ColorLocation,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(RenderVertex),
            (void*)12);

    glBindVertexArray(0);
    glBindBuffer(GL_VERTEX_ARRAY, 0);

    viewMatrixLocation = glGetUniformLocation(shader->ProgramId(), "view");
}

void LineRenderer::DrawLine(const Vector3& start, const Vector3& end, Color color)
{
    auto colorv4 = color.ToVector4();

    vertices.emplace_back(start, colorv4);
    vertices.emplace_back(end, colorv4);
}

void LineRenderer::Render(const glm::mat4& viewMatrix)
{
    glBindVertexArray(voa);
    shader->Use();

    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);

    for(int i = 0; i < vertices.size(); i += MaxVerticesPerBatch)
    {
        int totalVertices = Min(MaxVerticesPerBatch, (int)vertices.size() - i);

        glBindBuffer(GL_ARRAY_BUFFER, vertexVbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(RenderVertex) * totalVertices, &vertices[i]);
        glDrawArrays(GL_LINES, 0, totalVertices);
    }

    vertices.clear();
}
