#include <box2d/box2d.h>
#include "TilemapEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"

void TilemapEntity::OnAdded(const EntityDictionary& properties)
{
    observedObjectType = 1;
    flags |= CastsShadows;
}

void TilemapEntity::Render(Renderer* renderer)
{
    _renderer.Render(renderer);
}

template<typename T>
b2Vec2 ToB2Vec2(const Vector2Template<T>& v)
{
    return b2Vec2(v.x, v.y);
}

void TilemapEntity::SetMapSegment(const MapSegment& mapSegment)
{
    static constexpr int MAX_POINTS = 32;

    auto rigidBody = AddComponent<RigidBodyComponent>();

    // Generate colliders polygonal colliders
    for (auto& polygon : mapSegment.polygonColliders)
    {
        b2Vec2 points[MAX_POINTS];
        for (int i = 0; i < polygon.points.size(); ++i)
        {
            points[i].Set(polygon.points[i].x, polygon.points[i].y);
        }
        b2PolygonShape shape;
        shape.Set(points, polygon.points.size());

        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        rigidBody->CreateFixture(fixtureDef);
    }

    // FIXME: the dimensions of the map should be calculated as part of the content pipeline
    if (!mapSegment.layers.empty())
    {
        auto pathFinder = scene->AddService<PathFinderService>(
            mapSegment.layers[0].tileMap.Rows(),
            mapSegment.layers[0].tileMap.Cols(),
            Vector2{ 32, 32 });

        for (auto& rectangle : mapSegment.colliders)
        {
            auto bounds = rectangle.As<float>();
            pathFinder->AddObstacle(bounds);
        }
    }

    for (auto& rectangle : mapSegment.colliders)
    {
        auto bounds = rectangle.As<float>();
        rigidBody->CreateBoxCollider(bounds.Size(), false, bounds.GetCenter());
    }

    _renderer.SetMapSegment(&mapSegment);
    _renderer.SetOffset(TopLeft());

    _mapSegment = &mapSegment;
}
