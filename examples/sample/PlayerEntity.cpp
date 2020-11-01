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
        position = GetSnapshotPosition(scene->timeSinceStart - 0.3);
    }

    renderer->RenderRectangle(Rectangle(position - Dimensions() / 2, Dimensions()), Color::CornflowerBlue(), -0.99);
}

void PlayerEntity::SetMoveDirection(Vector2 direction)
{
    rigidBody->SetVelocity(direction);
}

Vector2 PlayerEntity::PositionAtFixedUpdateId(int fixedUpdateId, int currentFixedUpdateId)
{
    for (auto it = commands.begin(); it != commands.end(); ++it)
    {
        auto& command = *it;

        if (command.fixedUpdateStartId >= fixedUpdateId)
        {
            auto next = it;
            ++next;

            auto& nextCommand = *next;

            Vector2 start = command.positionAtStartOfCommand;
            Vector2 end;
            float t;

            if (next == commands.end() || nextCommand.status == PlayerCommandStatus::NotStarted)
            {
                // We don't have a next command to interpolate to, so use where we are now
                end = Center();
                float diff = currentFixedUpdateId - command.fixedUpdateStartId;
                t = (fixedUpdateId - command.fixedUpdateStartId) / diff;
            }
            else
            {
                end = nextCommand.positionAtStartOfCommand;
                float diff = nextCommand.fixedUpdateStartId - command.fixedUpdateStartId;
                t = (fixedUpdateId - command.fixedUpdateStartId) / diff;
            }

            return Lerp(start, end, t);
        }
    }

    return Center();
}

PlayerCommand* PlayerEntity::GetCommandById(int id)
{
    for (auto& command : commands)
    {
        if (command.id == id)
        {
            return &command;
        }
    }

    return nullptr;
}

Vector2 PlayerEntity::GetSnapshotPosition(float time)
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

    return Center();
}

void PlayerEntity::AddSnapshot(const PlayerSnapshot& snapshot)
{
    if (snapshots.size() > 0 && snapshots[snapshots.size() - 1].commandId == snapshot.commandId)
    {
        // Duplicate snapshot
        return;
    }

    snapshots.push_back(snapshot);
}