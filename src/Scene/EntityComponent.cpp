#include "EntityComponent.hpp"
#include "Entity.hpp"

Scene* IEntityComponent::GetScene() const
{
    return owner->scene;
}
