#include "PlayerEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Renderer/Renderer.hpp"

void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    rigidBody = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    rigidBody->CreateBoxCollider(Dimensions());

    scene->GetCameraFollower()->CenterOn(Center());
    scene->GetCameraFollower()->FollowEntity(this);

    scene->SendEvent(PlayerAddedToGame(this));
}

void PlayerEntity::OnDestroyed()
{
    scene->SendEvent(PlayerRemovedFromGame(this));
}

void PlayerEntity::Render(Renderer* renderer)
{
    renderer->RenderRectangle(Bounds(), Color::CornflowerBlue(), -0.99);
}

void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}
