#include "CastleEntity.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Physics/PathFinding.hpp"

void CastleEntity::OnAdded(const EntityDictionary& properties)
{
    auto spriteName = StringId(properties.GetValueOrDefault("sprite", "castle"));
    spriteComponent = AddComponent<SpriteComponent>("sprite", spriteName);

    spriteComponent->scale = Vector2(5.0f);

    auto rigidBody = AddComponent<RigidBodyComponent>("rb", b2_staticBody);
    Vector2 size{ 67 * 5, 55 * 5 };
    rigidBody->CreateBoxCollider(size);

    scene->GetService<PathFinderService>()->AddObstacle(Rectangle(Center() - size / 2, size));
}

void CastleEntity::OnEvent(const IEntityEvent& ev)
{
    if(ev.Is<ContactBeginEvent>())
    {
        _colorChangeTime = 1;
        spriteComponent->blendColor = Color(255, 0, 0, 128);
    }
}

void CastleEntity::Update(float deltaTime)
{
    _colorChangeTime -= deltaTime;

    if(_colorChangeTime < 0)
    {
        spriteComponent->blendColor = Color(0, 0, 0, 0);
    }
}
