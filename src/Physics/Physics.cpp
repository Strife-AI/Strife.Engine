#include "Physics.hpp"

#include <box2d/b2_contact.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_world.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_edge_shape.h>

#include "Renderer.hpp"
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

void CollisionManager::RenderColliderOutlines(Renderer* renderer)
{
    for (auto body = _world->GetBodyList(); body != nullptr; body = body->GetNext())
    {
        for (auto fixture = body->GetFixtureList(); fixture != nullptr; fixture = fixture->GetNext())
        {
            if (fixture->IsSensor()) continue;;

            auto shape = fixture->GetShape();
            switch (shape->GetType())
            {
            case b2Shape::e_circle:
            {
                auto circle = static_cast<b2CircleShape*>(shape);

                renderer->RenderCircleOutline(Scene::Box2DToPixel(body->GetWorldPoint(circle->m_p)), circle->m_radius * Scene::Box2DToPixelsRatio.x, Color(0, 255, 0), 0);// FIXME MW DebugRenderLayer);
                break;
            }
            case b2Shape::e_edge:
            {
                auto edge = static_cast<b2EdgeShape*>(shape);
                renderer->RenderLine(
                    Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex1)),
                    Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex2)),
                    Color(255, 255, 255),
                    0);//DebugRenderLayer); FIXME MW

                break;
            }
            case b2Shape::e_polygon:
            {
                auto polygon = static_cast<b2PolygonShape*>(shape);

                for (int i = 0; i < polygon->m_count; ++i)
                {
                    auto vertex = Scene::Box2DToPixel(polygon->m_vertices[i] + body->GetPosition());
                    auto next = Scene::Box2DToPixel(polygon->m_vertices[(i + 1) % polygon->m_count] + body->GetPosition());

                    renderer->RenderLine(vertex, next, Color::Red(), 0);// FIXME MW DebugRenderLayer);
                }

                break;
            }
            case b2Shape::e_chain: break;
            case b2Shape::e_typeCount: break;
            default:;
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
