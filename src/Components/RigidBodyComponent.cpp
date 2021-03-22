#include "RigidBodyComponent.hpp"

#include <box2d/b2_circle_shape.h>
#include <box2d/b2_edge_shape.h>
#include <box2d/b2_polygon_shape.h>

#include "Scene/Scene.hpp"

RigidBodyComponent::RigidBodyComponent(b2BodyType type)
{
    this->type = type;
}

void RigidBodyComponent::OnAdded()
{
    b2BodyDef bodyDef;
    bodyDef.type = type;
    CreateBody(bodyDef);
}

void RigidBodyComponent::SetVelocity(const Vector2& velocity)
{
    auto scaled = velocity * Scene::PixelsToBox2DRatio;
    body->SetLinearVelocity(b2Vec2(scaled.x, scaled.y));
}

Vector2 RigidBodyComponent::GetVelocity() const
{
    auto velocity = body->GetLinearVelocity();
    return Vector2(velocity.x, velocity.y) * Scene::Box2DToPixelsRatio;
}

void RigidBodyComponent::ApplyForce(const Vector2& force)
{
    auto box2dForce = b2Vec2(force.x * Scene::PixelsToBox2DRatio.x, force.y * Scene::PixelsToBox2DRatio.y);
    body->ApplyForceToCenter(box2dForce, true);
}

void RigidBodyComponent::ApplyForce(const Vector2& force, const Vector2& position)
{
    auto box2dForce = b2Vec2(force.x * Scene::PixelsToBox2DRatio.x, force.y * Scene::PixelsToBox2DRatio.y);
    auto box2dPosition = b2Vec2(position.x * Scene::PixelsToBox2DRatio.x, position.y * Scene::PixelsToBox2DRatio.y);
    body->ApplyForce(box2dForce, body->GetWorldPoint(box2dPosition), true);
}

void ScaleToBox2D(b2Vec2& v)
{
    v = b2Vec2(v.x * Scene::PixelsToBox2DRatio.x, v.y * Scene::PixelsToBox2DRatio.y);
}

void RigidBodyComponent::OnOwnerMoved()
{
    body->SetTransform(
        Scene::PixelToBox2D(owner->Center()),
        owner->Rotation());
}

void RigidBodyComponent::OnRemoved()
{
    GetScene()->GetWorld()->DestroyBody(body);
}

b2Body* RigidBodyComponent::CreateBody(b2BodyDef& bodyDef, bool createAtCurrentPosition)
{
    if (body != nullptr)
    {
        GetScene()->GetWorld()->DestroyBody(body);
    }

    if (createAtCurrentPosition)
    {
        auto ownerPosition = owner->Center();
        bodyDef.position = b2Vec2(ownerPosition.x, ownerPosition.y);
    }

    ScaleToBox2D(bodyDef.position);
    ScaleToBox2D(bodyDef.linearVelocity);

    bodyDef.userData.pointer = reinterpret_cast<uintptr_t>(this);

    body = GetScene()->GetWorld()->CreateBody(&bodyDef);

    return body;
}

void RigidBodyComponent::ResetBody()
{
    b2BodyType oldType = b2_staticBody;

    if (body != nullptr)
    {
        oldType = body->GetType();
        GetScene()->GetWorld()->DestroyBody(body);
    }

    b2BodyDef bodyDef;
    bodyDef.type = oldType;
    CreateBody(bodyDef);
}

b2Fixture* RigidBodyComponent::CreateBoxCollider(Vector2 size, bool isTrigger, Vector2 offset)
{
    b2PolygonShape shape;
    shape.SetAsBox(size.x / 2, size.y / 2, b2Vec2(offset.x, offset.y), 0);

    b2FixtureDef fixtureDef;
    fixtureDef.isSensor = isTrigger;
    fixtureDef.shape = &shape;

    return CreateFixture(fixtureDef);
}

b2Fixture* RigidBodyComponent::CreateCircleCollider(float radius, bool isTrigger, Vector2 offset)
{
    b2CircleShape circle;
    circle.m_radius = radius;
    circle.m_p = b2Vec2(offset.x, offset.y);

    b2FixtureDef fixtureDef;
    fixtureDef.isSensor = isTrigger;
    fixtureDef.shape = &circle;

    return CreateFixture(fixtureDef);
}

b2Fixture* RigidBodyComponent::CreateLineCollider(Vector2 start, Vector2 end, bool isTrigger)
{
    b2EdgeShape edge;

    edge.SetTwoSided(b2Vec2(start.x, start.y), b2Vec2(end.x, end.y));

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &edge;
    fixtureDef.isSensor = isTrigger;

    return CreateFixture(fixtureDef);
}

b2Fixture* RigidBodyComponent::CreateFixture(b2FixtureDef& fixtureDef)
{
    auto shape = const_cast<b2Shape*>(fixtureDef.shape);

    switch (shape->GetType())
    {
    case b2Shape::e_polygon:
    {
        auto polygon = static_cast<b2PolygonShape*>(shape);

        b2Vec2 vertices[b2_maxPolygonVertices];

        for (int i = 0; i < polygon->m_count; ++i)
        {
            vertices[i] = polygon->m_vertices[i];
            ScaleToBox2D(vertices[i]);
        }

        polygon->Set(vertices, polygon->m_count);

        break;
    }

    case b2Shape::e_edge:
    {
        auto edge = static_cast<b2EdgeShape*>(shape);

        ScaleToBox2D(edge->m_vertex1);
        ScaleToBox2D(edge->m_vertex2);

        edge->SetTwoSided(edge->m_vertex1, edge->m_vertex2);

        break;
    }

    case b2Shape::e_circle:
    {
        auto circle = static_cast<b2CircleShape*>(shape);

        ScaleToBox2D(circle->m_p);
        circle->m_radius *= Scene::PixelsToBox2DRatio.x;

        break;
    }

    default:
        FatalError("Shape is not supported; bother Michael to add it");
    }

    auto fixture = body->CreateFixture(&fixtureDef);
    fixture->SetDensity(1);
    body->ResetMassData();

    return fixture;
}

b2Fixture* RigidBodyComponent::CreatePolygonCollider(gsl::span<Vector2> points, bool isTrigger)
{
    b2PolygonShape shape;
    shape.m_count = points.size();

    for (int i = 0; i < points.size(); ++i)
    {
        shape.m_vertices[i] = b2Vec2(points[i].x, points[i].y);
    }

    b2FixtureDef fixtureDef;
    fixtureDef.isSensor = isTrigger;
    fixtureDef.shape = &shape;
    fixtureDef.isSensor = isTrigger;

    return CreateFixture(fixtureDef);
}

RigidBodyComponent* RigidBodyComponent::FromB2Body(b2Body* body)
{
    return reinterpret_cast<RigidBodyComponent*>(body->GetUserData().pointer);
}
