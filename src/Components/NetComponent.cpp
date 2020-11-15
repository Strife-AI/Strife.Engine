#include "NetComponent.hpp"

#include <slikenet/BitStream.h>


#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"

Vector2 GetDirectionFromKeyBits(unsigned keyBits)
{
    Vector2 moveDirection;

    if (keyBits & 1) moveDirection.x -= 1;
    if (keyBits & 2) moveDirection.x += 1;
    if (keyBits & 4) moveDirection.y -= 1;
    if (keyBits & 8) moveDirection.y += 1;

    return moveDirection;
}

void NetComponent::OnAdded()
{
    GetScene()->replicationManager.AddNetComponent(this);
}

void NetComponent::OnRemoved()
{
    GetScene()->replicationManager.RemoveNetComponent(this);
}

Vector2 NetComponent::GetSnapshotPosition(float time)
{
    for (int i = 0; i < (int)snapshots.size() - 1; ++i)
    {
        if (snapshots[i + 1].time > time)
        {
            return Lerp(
                snapshots[i].position,
                snapshots[i + 1].position,
                (time - snapshots[i].time) / (snapshots[i + 1].time - snapshots[i].time));
        }
    }

    return snapshots.size() > 0
        ? snapshots[snapshots.size() - 1].position
        : owner->Center();
}

void NetComponent::AddSnapshot(const PlayerSnapshot& snapshot)
{
    if (snapshots.size() > 0 && snapshots[snapshots.size() - 1].commandId >= snapshot.commandId)
    {
        // Duplicate snapshot
        return;
    }

    snapshots.push_back(snapshot);
}