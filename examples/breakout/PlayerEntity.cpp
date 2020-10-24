#include "PlayerEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Renderer/Renderer.hpp"

InputButton g_upButton(SDL_SCANCODE_W);
InputButton g_downButton(SDL_SCANCODE_S);
InputButton g_leftButton(SDL_SCANCODE_A);
InputButton g_rightButton(SDL_SCANCODE_D);

void PlayerEntity::OnAdded(const EntityDictionary& properties)
{
    rigidBody = AddComponent<RigidBodyComponent>("rb", b2_dynamicBody);
    rigidBody->CreateBoxCollider(Dimensions());

    scene->GetCameraFollower()->CenterOn(Center());
    scene->GetCameraFollower()->FollowEntity(this);
}

void PlayerEntity::Render(Renderer* renderer)
{
    renderer->RenderRectangle(Bounds(), Color::CornflowerBlue(), -0.99);
}

void PlayerEntity::Update(float deltaTime)
{
    auto moveDirection = Vector2(
        g_leftButton.IsDown() ? -1 : g_rightButton.IsDown() ? 1 : 0,
        g_upButton.IsDown() ? -1 : g_downButton.IsDown() ? 1 : 0);

    float moveSpeed = 300;
    rigidBody->SetVelocity(moveDirection * moveSpeed);
}
