#include "Physics.hpp"

#include <box2d/b2_contact.h>


#include "Components/RigidBodyComponent.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

void MoveEntityRecursive(RigidBodyComponent* rigidBody, Vector2 offset)
{
    auto entity = rigidBody->owner;
    auto position = entity->Center();
    entity->_position = position + offset;
    entity->_rotation = rigidBody->body->GetAngle();
    entity->NotifyMovement();

    for(auto child = entity->children; child != nullptr; child = child->nextSibling)
    {
        // FIXME: this is really inefficient
        auto childRigidBody = child->GetComponent<RigidBodyComponent>();
        MoveEntityRecursive(childRigidBody, offset);
    }
}

void CollisionManager::UpdateEntityPositions()
{
    for (auto body = _world->GetBodyList(); body != nullptr; body = body->GetNext())
    {
        auto data = body->GetUserData();

        if (data != nullptr)
        {
            auto rigidBody = reinterpret_cast<RigidBodyComponent*>(data);
            auto entity = rigidBody->owner;

            auto oldPosition = entity->Center();

            if (entity->flags & WasTeleported)
            {
                auto newPosition = entity->TopLeft();
                auto scaled = (newPosition + entity->Dimensions() / 2) * Scene::PixelsToBox2DRatio;
                auto angle = body->GetAngle();

                body->SetTransform(b2Vec2(scaled.x, scaled.y), angle);

                entity->flags &= ~WasTeleported;

                entity->SendEvent(EntityTeleportedEvent());
            }
            else
            {
                auto offset = Scene::Box2DToPixel(body->GetPosition()) - oldPosition;

                if (offset != Vector2::Zero())
                {
                    MoveEntityRecursive(rigidBody, offset);
                }
            }
        }
    }
}

void CollisionManager::BeginContact(b2Contact* contact)
{
    b2WorldManifold manifold;
    contact->GetWorldManifold(&manifold);
    Vector2 normal = Vector2(manifold.normal.x, manifold.normal.y);

    auto entityA = ColliderHandle(contact->GetFixtureA());
    auto entityB = ColliderHandle(contact->GetFixtureB());

    if (entityA.OwningEntity() != entityB.OwningEntity())
    {
        entityA.OwningEntity()->SendEvent(ContactBeginEvent(entityA, entityB, -normal, contact));
        entityB.OwningEntity()->SendEvent(ContactBeginEvent(entityB, entityA, normal, contact));
    }
}

void CollisionManager::EndContact(b2Contact* contact)
{
    auto entityA = ColliderHandle(contact->GetFixtureA());
    auto entityB = ColliderHandle(contact->GetFixtureB());

    if (entityA.OwningEntity() != entityB.OwningEntity())
    {
        entityA.OwningEntity()->SendEvent(ContactEndEvent(entityA, entityB));
        entityB.OwningEntity()->SendEvent(ContactEndEvent(entityB, entityA));
    }
}

void CollisionManager::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
    auto entityA = ColliderHandle(contact->GetFixtureA());
    auto entityB = ColliderHandle(contact->GetFixtureB());

    if (entityA.OwningEntity() != entityB.OwningEntity())
    {
        entityA.OwningEntity()->SendEvent(PreSolveEvent(entityA, entityB, contact));
        entityB.OwningEntity()->SendEvent(PreSolveEvent(entityB, entityA, contact));
    }
}

float FindClosestRaycastCallback::ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction)
{
    if ((!allowTriggers && fixture->IsSensor())
        || ColliderHandle(fixture).OwningEntity() == self
        || includeFixture != nullptr && !includeFixture({ fixture }))
    {
        return lastFraction;
    }
    else
    {
        lastFraction = fraction;
        closestPoint = point;
        closestFixture = fixture;
        closestNormal = normal;

        return fraction;
    }
}
