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
#include "PathFinding.hpp"

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
        auto data = body->GetUserData().pointer;
        auto rigidBody = reinterpret_cast<RigidBodyComponent*>(data);

        if (rigidBody != nullptr)
        {

            auto entity = rigidBody->owner;

            auto oldPosition = entity->Center();

            if (entity->flags.HasFlag(EntityFlags::WasTeleported))
            {
                auto newPosition = entity->TopLeft();
                auto scaled = (newPosition + entity->Dimensions() / 2) * Scene::PixelsToBox2DRatio;
                auto angle = body->GetAngle();

                body->SetTransform(b2Vec2(scaled.x, scaled.y), angle);

                entity->flags.ResetFlag(EntityFlags::WasTeleported);

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

static void DrawColliderLine(Vector2 worldStart, Vector2 worldEnd, Renderer* renderer, Scene* scene, Color color)
{
    auto start = worldStart; //scene->isometricSettings.WorldToScreen(worldStart);
    auto end = worldEnd; scene->isometricSettings.WorldToScreen(worldEnd);

    renderer->RenderLine(start, end, color, -1);
}

static void GenerateCircle(Vector2 center, float radius, gsl::span<Vector2> outVertices)
{
    float dAngle = 2 * 3.1415926535 / outVertices.size();

    for (int i = 0; i < outVertices.size(); ++i)
    {
        auto angle = dAngle * i;
        outVertices[i] = center + Vector2(cos(angle), sin(angle)) * radius;
    }
}

void CollisionManager::RenderColliderOutlines(Renderer* renderer)
{
    for (auto body = _world->GetBodyList(); body != nullptr; body = body->GetNext())
    {
        for (auto fixture = body->GetFixtureList(); fixture != nullptr; fixture = fixture->GetNext())
        {
            if (fixture->IsSensor()) continue;;

            auto scene = ColliderHandle(fixture).OwningEntity()->scene;

            auto shape = fixture->GetShape();
            switch (shape->GetType())
            {
            case b2Shape::e_circle:
            {
                auto circle = static_cast<b2CircleShape*>(shape);
                auto center = Scene::Box2DToPixel(body->GetWorldPoint(circle->m_p));
                auto radius = circle->m_radius * Scene::Box2DToPixelsRatio.x;

                const int totalVertices = 32;
                Vector2 vertices[totalVertices];
                GenerateCircle(center, radius, vertices);

                for (int i = 0; i < totalVertices; ++i)
                {
                    DrawColliderLine(vertices[i], vertices[(i + 1) % totalVertices], renderer, scene, Color::Green());
                }

                break;
            }
            case b2Shape::e_edge:
            {
                auto edge = static_cast<b2EdgeShape*>(shape);
                auto start = Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex1));
                auto end = Scene::Box2DToPixel(body->GetWorldPoint(edge->m_vertex2));

                DrawColliderLine(start, end, renderer, scene, Color::White());

                break;
            }
            case b2Shape::e_polygon:
            {
                auto polygon = static_cast<b2PolygonShape*>(shape);

                for (int i = 0; i < polygon->m_count; ++i)
                {
                    auto vertex = Scene::Box2DToPixel(polygon->m_vertices[i] + body->GetPosition());
                    auto next = Scene::Box2DToPixel(polygon->m_vertices[(i + 1) % polygon->m_count] + body->GetPosition());

                    DrawColliderLine(vertex, next, renderer, scene, Color::Red());
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
