#include "PlayerEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Renderer/Renderer.hpp"

void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    auto rigidBody = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    rigidBody->CreateBoxCollider(Dimensions());

    scene->GetCameraFollower()->CenterOn(Center());
    scene->GetCameraFollower()->FollowMouse();
}

void PlayerEntity::Render(Renderer* renderer)
{
    renderer->RenderRectangle(Bounds(), Color::Red(), -0.99);
}