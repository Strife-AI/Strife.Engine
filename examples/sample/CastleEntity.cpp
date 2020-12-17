#include "CastleEntity.hpp"

#include "Engine.hpp"
#include "PlayerEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Physics/PathFinding.hpp"
#include "Net/ReplicationManager.hpp"

void CastleEntity::OnAdded(const EntityDictionary& properties)
{
    auto spriteName = StringId(properties.GetValueOrDefault("sprite", "castle"));
    spriteComponent = AddComponent<SpriteComponent>(spriteName);

    if(!scene->isServer && !properties.HasProperty("net"))
    {
        Destroy();
        return;
    }

    spriteComponent->scale = Vector2(5.0f);

    auto rigidBody = AddComponent<RigidBodyComponent>(b2_staticBody);
    Vector2 size{ 67 * 5, 55 * 5 };

    SetDimensions(size);

    rigidBody->CreateBoxCollider(size);

    rigidBody->CreateBoxCollider({ 600, 600 }, true);

    scene->GetService<PathFinderService>()->AddObstacle(Rectangle(Center() - size / 2, size));

    net = AddComponent<NetComponent>();

    auto health = AddComponent<HealthBarComponent>();
    health->offsetFromCenter = -size.YVector() / 2 - Vector2(0, 5);

    auto offset = size / 2 + Vector2(40, 40);

    _spawnSlots[0] = Center() + offset.XVector();
    _spawnSlots[1] = Center() - offset.XVector();
    _spawnSlots[2] = Center() + offset.YVector();
    _spawnSlots[3] = Center() - offset.YVector();

    if(!scene->isServer)
        scene->GetLightManager()->AddLight(&_light);

    _light.position = Center();
    _light.intensity = 0;
    _light.color = Color::White();
    _light.maxDistance = 500;
}

void CastleEntity::Update(float deltaTime)
{

}

void CastleEntity::ReceiveServerEvent(const IEntityEvent& ev)
{

}

void CastleEntity::SpawnPlayer()
{
    auto position = _spawnSlots[_nextSpawnSlotId];
    _nextSpawnSlotId = (_nextSpawnSlotId + 1) % 4;

    EntityProperty properties[] = {
            EntityProperty::EntityType<PlayerEntity>(),
            { "position", position },
            { "dimensions", { 30, 30 } },
    };

    auto player = static_cast<PlayerEntity*>(scene->CreateEntity(EntityDictionary(properties)));

    player->GetComponent<NetComponent>()->ownerClientId = net->ownerClientId;

    _light.intensity = 0.5;
}

void CastleEntity::OnDestroyed()
{
    scene->GetLightManager()->RemoveLight(&_light);
}

void CastleEntity::ReceiveEvent(const IEntityEvent& ev)
{
    if (ev.Is<SpawnedOnClientEvent>())
    {
        if (net->ownerClientId == scene->replicationManager->localClientId)
        {
            scene->GetCameraFollower()->FollowMouse();
            scene->GetCameraFollower()->CenterOn(Center());
        }
    }
}
