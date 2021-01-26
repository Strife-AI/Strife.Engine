#pragma once
#include <box2d/b2_body.h>

#include "Math/Vector2.hpp"
#include "Scene/EntityComponent.hpp"
#include "gsl/span"

DEFINE_COMPONENT(RigidBodyComponent)
{
    RigidBodyComponent(b2BodyType type = b2_staticBody);
    void OnAdded() override;

    void OnOwnerMoved() override;
    void OnRemoved() override;

    b2Body* CreateBody(b2BodyDef& bodyDef, bool createAtCurrentPosition = true);

    void ResetBody();

    void SetVelocity(const Vector2& velocity);
    Vector2 GetVelocity() const;
    void ApplyForce(const Vector2& force);
    void ApplyForce(const Vector2& force, const Vector2 & position);

    b2Fixture* CreateBoxCollider(Vector2 size, bool isTrigger = false, Vector2 offset = Vector2(0, 0));
    b2Fixture* CreateCircleCollider(float radius, bool isTrigger = false, Vector2 offset = Vector2(0, 0));
    b2Fixture* CreateLineCollider(Vector2 start, Vector2 end, bool isTrigger = false);
    b2Fixture* CreatePolygonCollider(gsl::span<Vector2> points, bool isTrigger = false);
    b2Fixture* CreateFixture(b2FixtureDef& fixtureDef);

    b2Body* body = nullptr;
    b2BodyType type;
};
