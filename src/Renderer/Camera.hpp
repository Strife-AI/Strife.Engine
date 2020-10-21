#pragma once

#include <Math/Vector2.hpp>
#include "glm/mat4x4.hpp"
#include "Math/Rectangle.hpp"

class Camera
{
public:
    Vector2 TopLeft() const { return Position() - ZoomedScreenSize() / 2; }
    Vector2 Position() const { return _position; }
    Vector2 ScreenSize() const { return _viewport; }
    Vector2 ZoomedScreenSize() const { return _viewport / _zoom; }

    float Zoom() const { return _zoom; }

    void SetScreenSize(Vector2 size);
    void SetPosition(Vector2 position);
    void SetZoom(float zoom);

    void RebuildViewMatrix();

    const glm::mat4& ViewMatrix() const { return _viewMatrix; }

    Rectangle Bounds() const
    {
        return Rectangle(TopLeft(), ZoomedScreenSize());
    }

    Vector2 ScreenToWorld(Vector2 screenPoint) const;
    Vector2 WorldToScreen(Vector2 worldPoint) const;

private:

    Vector2 _position;
    float _zoom = 1;
    Vector2 _viewport;

    glm::mat4 _viewMatrix;
};

