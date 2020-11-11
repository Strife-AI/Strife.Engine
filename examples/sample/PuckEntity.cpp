#include "PuckEntity.hpp"

#include "Components/RigidBodyComponent.hpp"
#include "Renderer/Renderer.hpp"

void PuckEntity::OnAdded(const EntityDictionary& properties)
{
    AddComponent<RigidBodyComponent>("rb", b2_dynamicBody)->CreateBoxCollider({ 15, 15 });
    AddComponent<NetComponent>();
}

void PuckEntity::Render(Renderer* renderer)
{
    renderer->RenderRectangle(Bounds(), Color::Red(), -1);
}
