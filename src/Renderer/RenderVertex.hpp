#pragma once

#include <Math/Vector3.hpp>
#include <Math/Vector4.hpp>
#include <Math/Vector2.hpp>

struct RenderVertex
{
    RenderVertex(const Vector3& position_, const Vector2& textureCoord_)
        : position(position_),
          textureCoord(textureCoord_)
    {
    }

    RenderVertex(const Vector3& position_, const Vector2& textureCoord_, const Vector4& color_)
        : position(position_),
        color(color_),
        textureCoord(textureCoord_)
    {
    }

    RenderVertex(const Vector3& position_, const Vector4& color_)
            : position(position_),
              color(color_)
    {
    }

    RenderVertex() = default;

    Vector3 position;
    Vector4 color;
    Vector2 textureCoord;
};