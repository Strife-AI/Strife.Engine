#include "NetComponent.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

bool ISyncVar::isServer = false;

Vector2 GetDirectionFromKeyBits(unsigned keyBits)
{
    Vector2 moveDirection;

    if (keyBits & 1) moveDirection.x -= 1;
    if (keyBits & 2) moveDirection.x += 1;
    if (keyBits & 4) moveDirection.y -= 1;
    if (keyBits & 8) moveDirection.y += 1;

    return moveDirection;
}

ISyncVar::ISyncVar(SyncVarUpdateFrequency frequency_)
    : frequency(frequency_)
{
    auto entity = Scene::entityUnderConstruction;
    if(entity == nullptr)
    {
        FatalError("Syncvars can only be added to an entity");
    }

    next = entity->syncVarHead;
    entity->syncVarHead = this;
}

void NetComponent::OnAdded()
{
    GetScene()->replicationManager.AddNetComponent(this);
}

void NetComponent::OnRemoved()
{
    GetScene()->replicationManager.RemoveNetComponent(this);
}