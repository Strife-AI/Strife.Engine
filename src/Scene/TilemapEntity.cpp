#include <box2d/box2d.h>
#include "TilemapEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"

void TilemapEntity::OnAdded(const EntityDictionary& properties)
{
    flags |= CastsShadows;

    if(!scene->isServer)
        scene->GetLightManager()->AddLight(&light);

    light.bounds = Rectangle(0, 0, 256 * 32, 256 * 32);
    light.color = Color(64, 64, 255);
    light.intensity = 0.25;
}

void TilemapEntity::OnDestroyed()
{
    if(!scene->isServer)
        scene->GetLightManager()->AddLight(&light);
}

void TilemapEntity::Render(Renderer* renderer)
{
    _renderer.Render(renderer);
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
