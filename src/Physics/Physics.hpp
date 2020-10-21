#pragma once
#include <functional>
#include <box2d/b2_fixture.h>
#include <box2d/b2_world.h>
#include <box2d/b2_world_callbacks.h>

#include "ColliderHandle.hpp"

class CollisionManager : public b2ContactListener
{
public:
    CollisionManager(b2World* world)
        : _world(world)
    {

    }

    void UpdateEntityPositions();

private:

    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;
    void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;

    b2World* _world;
};

struct Entity;

class FindClosestRaycastCallback : public b2RayCastCallback
{
public:
    FindClosestRaycastCallback(Entity* self_, bool allowTriggers_, const std::function<bool(ColliderHandle handle)>& includeFixture_ = nullptr)
        : self(self_),
        allowTriggers(allowTriggers_),
        includeFixture(includeFixture_)
    {
        
    }

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override;

    Entity* self;
    b2Fixture* closestFixture;
    b2Vec2 closestPoint;
    float lastFraction = 1;
    b2Vec2 closestNormal;
    bool allowTriggers;
    const std::function<bool(ColliderHandle handle)>& includeFixture;
};