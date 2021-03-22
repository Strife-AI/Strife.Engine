#include "ColliderHandle.hpp"


#include <box2d/b2_circle_shape.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_edge_shape.h>


#include "Components/RigidBodyComponent.hpp"
#include "Scene/Scene.hpp"

Entity* ColliderHandle::OwningEntity() const
{
    return OwningRigidBody()->owner;
}

RigidBodyComponent* ColliderHandle::OwningRigidBody() const
{
    return reinterpret_cast<RigidBodyComponent*>(_fixture->GetBody()->GetUserData().pointer);
}

Rectangle ColliderHandle::Bounds() const
{
    auto shape = _fixture->GetShape();
    auto body = _fixture->GetBody();

    switch (shape->GetType())
    {
    case b2Shape::e_polygon:
    {
        auto polygon = static_cast<const b2PolygonShape*>(shape);
        Vector2 min = Vector2(1000000, 1000000);
        Vector2 max = -min;

        for (int i = 0; i < polygon->m_count; ++i)
        {
            auto v = body->GetWorldPoint(polygon->m_vertices[i]);
            auto v2 = Vector2(v.x, v.y);

            min = min.Min(v2);
            max = max.Max(v2);
        }

        return Rectangle(min, max - min).Scale(Scene::Box2DToPixelsRatio);
    }

    case b2Shape::e_circle:
    {
        auto circle = static_cast<const b2CircleShape*>(shape);
        auto center = body->GetWorldPoint(circle->m_p);
        auto center2 = Vector2(center.x, center.y);
        auto size = Vector2(circle->m_radius, circle->m_radius);

        return Rectangle(center2 - size, size * 2).Scale(Scene::Box2DToPixelsRatio);
    }

    case b2Shape::e_edge:
    {
        auto edge = static_cast<const b2EdgeShape*>(shape);
        auto a = Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex1));
        auto b = Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex2));

        return Rectangle::FromPoints(a, b);
    }


    default:
        FatalError("Unsupported fixture type: %d", (int)shape->GetType());
    }
}
