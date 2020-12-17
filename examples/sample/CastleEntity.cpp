#include "CastleEntity.hpp"

#include "Engine.hpp"
#include "PlayerEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Physics/PathFinding.hpp"
#include "Net/ReplicationManager.hpp"
#include "MessageHud.hpp"

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
    health->maxHealth = 500;
    health->health.SetValue(500);

    auto offset = size / 2 + Vector2(40, 40);

    _spawnSlots[0] = Center() + offset.XVector();
    _spawnSlots[1] = Center() - offset.XVector();
    _spawnSlots[2] = Center() + offset.YVector();
    _spawnSlots[3] = Center() - offset.YVector();

    if(!scene->isServer)
        scene->GetLightManager()->AddLight(&_light);

    _light.position = Center();
    _light.intensity = 0;
    _light.maxDistance = 500;
}

void CastleEntity::Update(float deltaTime)
{
    if(net == nullptr)
    {
        return;
    }

    _light.color = scene->replicationManager->localClientId == net->ownerClientId
        ? Color::Green()
        : Color::White();

    if(net->ownerClientId >= 0)
        _light.intensity = 0.5;
    else
        _light.intensity = 0;
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
}

void CastleEntity::OnDestroyed()
{
    scene->GetLightManager()->RemoveLight(&_light);

    if(scene->isServer && net != nullptr && net->ownerClientId >= 0)
    {
        for(auto player : scene->GetEntitiesOfType<PlayerEntity>())
        {
            if(player->net->ownerClientId == net->ownerClientId)
            {
                player->Destroy();
            }
        }

        auto& selfName = scene->replicationManager->GetClient(net->ownerClientId).clientName;
        scene->SendEvent(BroadcastToClientMessage(selfName + " has been eliminated!"));
    }
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
