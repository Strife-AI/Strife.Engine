#include "NetComponent.hpp"

#include "Scene/Entity.hpp"

PlayerCommand* NetComponent::GetCommandById(int id)
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

Vector2 NetComponent::PositionAtFixedUpdateId(int fixedUpdateId, int currentFixedUpdateId)
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
                end = owner->Center();
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

    return owner->Center();
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

    return owner->Center();
}

void NetComponent::AddSnapshot(const PlayerSnapshot& snapshot)
{
    if (snapshots.size() > 0 && snapshots[snapshots.size() - 1].commandId == snapshot.commandId)
    {
        // Duplicate snapshot
        return;
    }

    snapshots.push_back(snapshot);
}