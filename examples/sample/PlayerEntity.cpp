#include "PlayerEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Renderer/Renderer.hpp"

void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    rigidBody = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    rigidBody->CreateBoxCollider(Dimensions());

    scene->SendEvent(PlayerAddedToGame(this));
}

void PlayerEntity::OnDestroyed()
{
    scene->SendEvent(PlayerRemovedFromGame(this));
}

void PlayerEntity::Render(Renderer* renderer)
{
    auto position = Center();

    if (!isClientPlayer)
    {
        position = GetSnapshotPosition(scene->timeSinceStart - 0.1);
        SetCenter(position);
    }


    renderer->RenderRectangle(Rectangle(position - Dimensions() / 2, Dimensions()), Color::CornflowerBlue(), -0.99);
}

void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}
