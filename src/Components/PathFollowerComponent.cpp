#include <LightAnimations.hpp>
#include <Memory/Util.hpp>
#include "PathFollowerComponent.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Renderer.hpp"
#include "Scene/TilemapEntity.hpp"

void PathFollowerComponent::OnAdded()
{
    currentLayer = GetScene()->isometricSettings.GetCurrentLayer(owner->Center());

    auto scene = GetScene();
    auto tile = scene->isometricSettings.ScreenToTile(owner->Center());
    pathFindingPosition = tile + Vector2(scene->isometricSettings.terrain[tile].height);

    GetScene()->GetService<PathFinderService>()->pathFollowers.push_back(this);
}

void PathFollowerComponent::OnRemoved()
{
    RemoveFromVector(GetScene()->GetService<PathFinderService>()->pathFollowers, this);
}

void PathFollowerComponent::FixedUpdate(float deltaTime)
{
    if (state == PathFollowerState::Stopped)
    {
        rigidBody->SetVelocity({ 0, 0 });
    }

    FollowFlowField();

    if (state == PathFollowerState::FollowingEntity)
    {
        auto scene = GetScene();
        UpdateFollowTarget(deltaTime, scene);
    }
}


void PathFollowerComponent::UpdateFollowTarget(float deltaTime, Scene* scene)
{
    Entity* target;
    if (entityToFollow.TryGetValue(target))
    {
        updateTargetTimer -= deltaTime;
        if (updateTargetTimer <= 0)
        {
            UpdateFlowField(target->Center());
            updateTargetTimer = 2;
        }
        else
        {
            currentPath.target = target->Center();
        }
    }
    else
    {
        state = PathFollowerState::Stopped;
    }
}

static Vector2 SetTileVectorLengthInWorldSpace(Vector2 tileSpace, float worldLength, IsometricSettings* isometricSettings)
{
    return isometricSettings->ScreenToTile(
        isometricSettings->TileToScreen(tileSpace).Normalize() * worldLength);
}

void PathFollowerComponent::FollowFlowField()
{
    auto scene = GetScene();
    Vector2 tileTarget;
    Vector2 velocity;

    pathFindingPosition = ToPathfinderPerspective(owner->Center());

    if (currentPath.type == PathFollowerPathType::None)
    {
        return;
    }
    else if (currentPath.type == PathFollowerPathType::FlowField)
    {
        if (currentPath.path.size() == 0 || currentPath.nextPathIndex >= (int)currentPath.path.size())
        {
            rigidBody->SetVelocity({ 0, 0 });
            return;
        }

        if (false && currentPath.flowField->grid[pathFindingPosition].hasLineOfSightToGoal && CanBeeline(owner->Center(), scene->isometricSettings.TileToScreen(currentPath.target)))
        {
            intermediateTarget = currentPath.target;
            velocity = (currentPath.target - pathFindingPosition);
        }
        else
        {
            if (currentPath.nextPathIndex == -1 || (intermediateTarget - pathFindingPosition).Length() < 0.2)
            {
                if (currentPath.nextPathIndex + 1 < currentPath.path.size())
                {
                    ++currentPath.nextPathIndex;
                }
            }

            tileTarget = currentPath.path[currentPath.nextPathIndex];

            velocity = (tileTarget - pathFindingPosition);

            intermediateTarget = tileTarget;
        }
    }
    else if (currentPath.type == PathFollowerPathType::Beeline)
    {
        velocity = (currentPath.target - pathFindingPosition);
    }

//    float dist = (currentPath.target - owner->Center()).Length();
//    if (dist > 20)
//    {
//        velocity = rigidBody->GetVelocity().SmoothDamp(
//            velocity,
//            acceleration,
//            0.05,
//            Scene::PhysicsDeltaTime);
//    }
//
//    if (dist <= 1)
//    {
//        velocity = { 0, 0 };
//        acceleration = { 0, 0 };
//        Stop(true);
//    }

    //rigidBody->SetVelocity(velocity);

    rigidBody->SetVelocity(scene->isometricSettings.TileToWorld(velocity).Normalize() * speed);

#if false
    auto velocityAtCorrectSpeed = SetTileVectorLengthInWorldSpace(velocity, speed, &scene->isometricSettings);

    pathFindingPosition += velocityAtCorrectSpeed * Scene::PhysicsDeltaTime;

    auto height = Vector2(scene->GetService<PathFinderService>()->GetCell(pathFindingPosition).height);
    auto targetPosition = scene->isometricSettings.TileToWorld(pathFindingPosition);

    if (scene->GetService<PathFinderService>()->GetCell(pathFindingPosition).ramp == ObstacleRampType::None)
    {
        targetPosition.y -= height.y * 32;
    }
    else
    {
        float frac = pathFindingPosition.x - floor(pathFindingPosition.x);
        targetPosition.y -= (1.0 - frac) * 32;
    }

    currentLayer.SetValue(2);

    owner->SetCenter(targetPosition);
#endif
}

void PathFollowerComponent::UpdateFlowField(Vector2 newTarget)
{
    auto targetPathFinderPerspective = ToPathfinderPerspective(newTarget);

    if (!CanBeeline(owner->Center(), newTarget))
    {
        currentTarget = targetPathFinderPerspective;
        intermediateTarget = targetPathFinderPerspective;
        currentPath.type = PathFollowerPathType::FlowField;
        currentPath.path.clear();

        auto centerPathFinderPerspective = ToPathfinderPerspective(owner->Center());
        GetScene()->GetService<PathFinderService>()->RequestFlowField(centerPathFinderPerspective, targetPathFinderPerspective, owner);
    }
    else
    {
        currentPath.target = targetPathFinderPerspective;
        currentPath.type = PathFollowerPathType::Beeline;
        currentPath.path.clear();
    }
}

void PathFollowerComponent::SetTarget(Vector2 position)
{
    state = PathFollowerState::MovingToTarget;
    UpdateFlowField(position);
}

void PathFollowerComponent::Stop(bool loseVelocity)
{
    state = PathFollowerState::Stopped;
    if (loseVelocity) rigidBody->SetVelocity({ 0, 0 });
    currentPath.type = PathFollowerPathType::None;
    currentPath.path.clear();

}

void PathFollowerComponent::ReceiveEvent(const IEntityEvent& ev)
{
    if (auto flowFieldReady = ev.Is<FlowFieldReadyEvent>())
    {
        //if (state == PathFollowerState::Stopped) return;
        if (!flowFieldReady->result->grid[ToPathfinderPerspective(currentTarget)].alreadyVisited)
        {
            // Failed to find a path
            //return;
        }

        auto flowField = flowFieldReady->result;
        auto cell = flowField->startCell;
        currentPath.path.clear();
        currentPath.nextPathIndex = -1;
        currentPath.flowField = flowField;

        Vector2 dir;
        do
        {
            dir = flowField->grid[cell].dir;

            if (dir == Vector2(0)) break;

            cell = cell + dir;
            currentPath.path.push_back(cell + Vector2(0.5));
        } while (true);

        currentPath.target = currentTarget;
        acceleration = { 0, 0 };
    }
}

void PathFollowerComponent::FollowEntity(Entity* entity, float minDistance)
{
    if (state == PathFollowerState::FollowingEntity && entity == entityToFollow.GetValueOrNull())
    {
        return;
    }

    state = PathFollowerState::FollowingEntity;
    UpdateFlowField(entity->Center());
    entityToFollow = entity;
    updateTargetTimer = 2;
}

Vector2 PathFollowerComponent::ToPathfinderPerspective(Vector2 position)
{
    auto scene = GetScene();
    if (scene->perspective == ScenePerspective::Orothgraphic) return position / Vector2(32, 32);    // TODO: don't hardcode
    else if (scene->perspective == ScenePerspective::Isometric)
    {
        return scene->isometricSettings.WorldToTile(position);
    }

    return position;
}

bool PathFollowerComponent::CanBeeline(Vector2 from, Vector2 to)
{
    return false;
    Vector2 checkPoints[5];
    auto scene = GetScene();
    auto toTile = scene->isometricSettings.ScreenToIntegerTile(to);
    scene->isometricSettings.GetTileBoundaries(toTile, checkPoints);
    checkPoints[4] = to;

    for (auto point : checkPoints)
    {
        RaycastResult result;
        if (scene->Raycast(owner->Center(), point, result, true, [=](const ColliderHandle& handle)
        {
            return handle.OwningEntity()->Is<TilemapEntity>()
               || !handle.IsTrigger();
        }))
        {
            return false;
        }
    }

    return true;
}

void PathFollowerComponent::Update(float deltaTime)
{
    if (owner->type != "player"_sid) return;

#if 1
    if (currentPath.flowField != nullptr)
    {
        for (int i = 0; i < currentPath.flowField->grid.Rows(); ++i)
        {
            for (int j = 0; j < currentPath.flowField->grid.Cols(); ++j)
            {
                Color c = Color::Black();

                if (Vector2(j, i) == owner->scene->isometricSettings.ScreenToIntegerTile(owner->Center()))
                {
                    c = Color::Yellow();
                }

                auto dir = currentPath.flowField->grid[Vector2(j, i)].dir * 16;
                auto result = Vector2((dir.x - dir.y), (dir.x + dir.y) / 2);
                auto start = owner->scene->isometricSettings.TileToScreen(Vector2(j + 0.5, i + 0.5));
                //Renderer::DrawDebugLine({ start, start + result, Color::Black() });

                if (currentPath.flowField->grid[Vector2(j, i)].hasLineOfSightToGoal)
                    Renderer::DrawDebugRectangle(Rectangle(start - Vector2(2, 2), Vector2(4, 4)), c);
            }
        }
    }
#endif
}
