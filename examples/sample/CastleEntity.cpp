#include "CastleEntity.hpp"

#include "Engine.hpp"
#include "PlayerEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Physics/PathFinding.hpp"

void CastleEntity::OnAdded(const EntityDictionary& properties)
{
    auto spriteName = StringId(properties.GetValueOrDefault("sprite", "castle"));
    spriteComponent = AddComponent<SpriteComponent>("sprite", spriteName);

    if(GetEngine()->GetNetworkManger()->IsClient() && !properties.HasProperty("net"))
    {
        Destroy();
    }

    spriteComponent->scale = Vector2(5.0f);

    auto rigidBody = AddComponent<RigidBodyComponent>("rb", b2_staticBody);
    Vector2 size{ 67 * 5, 55 * 5 };
    rigidBody->CreateBoxCollider(size);

    rigidBody->CreateBoxCollider({ 600, 600 }, true);

    scene->GetService<PathFinderService>()->AddObstacle(Rectangle(Center() - size / 2, size));

    AddComponent<NetComponent>();
}

void CastleEntity::ReceiveServerEvent(const IEntityEvent& ev)
{
    if (auto contactBegin = ev.Is<ContactBeginEvent>())
    {
        ++_playerCount;
        _colorChangeTime = 1;
        _drawRed = true;
        spriteComponent->blendColor = Color(255, 0, 0, 128);

        PlayerEntity* player;
        if(contactBegin->OtherIs(player))
        {
            player->health -= 10;
        }
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

void CastleEntity::DoNetSerialize(NetSerializer& serializer)
{
    if(serializer.Add(_drawRed))
    {
        spriteComponent->blendColor = _drawRed
            ? Color(255, 0, 0, 128)
            : Color(0, 0, 0, 0);
    }
}
