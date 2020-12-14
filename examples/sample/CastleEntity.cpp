#include "CastleEntity.hpp"

#include "Engine.hpp"
#include "PlayerEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Physics/PathFinding.hpp"

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
    rigidBody->CreateBoxCollider(size);

    rigidBody->CreateBoxCollider({ 600, 600 }, true);

    scene->GetService<PathFinderService>()->AddObstacle(Rectangle(Center() - size / 2, size));

    AddComponent<NetComponent>();
}

void CastleEntity::Update(float deltaTime)
{
    if(_drawRed.currentValue)
    {
        spriteComponent->blendColor = Color(255, 0, 0, 128);
    }
    else
    {
        spriteComponent->blendColor = Color(0, 0, 0, 0);
    }
}

void CastleEntity::ReceiveServerEvent(const IEntityEvent& ev)
{
    if (auto contactBegin = ev.Is<ContactBeginEvent>())
    {
        ++_playerCount;
        _colorChangeTime = 1;
        _drawRed = true;
        spriteComponent->blendColor = Color(255, 0, 0, 128);
    }
    else if(ev.Is<ContactEndEvent>())
    {
        if(--_playerCount == 0)
        {
            _drawRed = false;
            spriteComponent->blendColor = Color(0, 0, 0, 0);
        }
    }
}