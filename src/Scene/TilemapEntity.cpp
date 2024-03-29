#include <box2d/box2d.h>
#include <glm/ext/matrix_transform.hpp>
#include "TilemapEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Physics/PathFinding.hpp"
#include "Renderer/Renderer.hpp"

void TilemapEntity::OnAdded()
{
    flags.SetFlag(EntityFlags::CastsShadows);

    if(!scene->isServer)
        scene->GetLightManager()->AddLight(&light);

    light.bounds = Rectangle(0, 0, 256 * 64, 256 * 64);
    light.color = Color(64, 64, 255);
    light.intensity = 0.5;
}

void TilemapEntity::OnDestroyed()
{
    if(!scene->isServer)
        scene->GetLightManager()->RemoveLight(&light);
}

void TilemapEntity::Render(Renderer* renderer)
{
    _renderer.Render(renderer);
}

void TilemapEntity::SetMapSegment(const MapSegment& mapSegment)
{
    static constexpr int MAX_POINTS = 8;

    auto rigidBody = AddComponent<RigidBodyComponent>();

    // Generate polygonal colliders
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

    PathFinderService* pathFinder = nullptr;

    // FIXME: the dimensions of the map should be calculated as part of the content pipeline
    if (!mapSegment.layers.empty())
    {
        pathFinder = scene->AddService<PathFinderService>(
            mapSegment.layers[0].tileMap.Rows(),
            mapSegment.layers[0].tileMap.Cols());
    }

    _renderer.SetMapSegment(&mapSegment, scene);
    _renderer.SetOffset(TopLeft());
    scene->isometricSettings.tileSize = mapSegment.layers[0].tileSize.AsVectorOfType<float>();

    if (scene->perspective == ScenePerspective::Isometric)
    {
        scene->isometricSettings.tileSize = Vector2(64, 32);
    }

    for (auto& entityDictionary : mapSegment.entities)
    {
        EntitySerializer entitySerializer;
        entitySerializer.properties = entityDictionary.properties;
        entitySerializer.mode = EntitySerializerMode::Read;

        scene->CreateEntity(entityDictionary.type, entitySerializer);
    }

    _mapSegment = &mapSegment;

    scene->isometricSettings.BuildFromMapSegment(mapSegment, pathFinder, this);
}

void WalkableTerrainEntity0::OnAdded() { rigidBody = AddComponent<RigidBodyComponent>(); }
void WalkableTerrainEntity1::OnAdded() { rigidBody = AddComponent<RigidBodyComponent>(); }
void WalkableTerrainEntity2::OnAdded() { rigidBody = AddComponent<RigidBodyComponent>(); }

void RampEntity::OnAdded()
{
    rigidBody = AddComponent<RigidBodyComponent>();
}
