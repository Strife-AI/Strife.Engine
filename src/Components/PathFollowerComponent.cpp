#include "PathFollowerComponent.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Renderer.hpp"
#include "Scene/TilemapEntity.hpp"

void PathFollowerComponent::OnAdded()
{
    currentLayer = GetScene()->isometricSettings.GetCurrentLayer(owner->Center());
}

void PathFollowerComponent::FixedUpdate(float deltaTime)
{
#if 0
    if (flowField != nullptr)
    {
        for (int i = 0; i < flowField->grid.Rows(); ++i)
        {
            for (int j = 0; j < flowField->grid.Cols(); ++j)
            {
                Color c = Color::Black();

                if (Vector2(j, i) == owner->scene->isometricSettings.WorldToIntegerTile(owner->Center()))
                {
                    c = Color::Yellow();
                }

                auto dir = flowField->grid[Vector2(j, i)].dir * 16;
                auto result = Vector2((dir.x - dir.y), (dir.x + dir.y) / 2);
                auto start = owner->scene->isometricSettings.TileToWorld(Vector2(j + 0.5, i + 0.5));
                Renderer::DrawDebugLine({ start, start + result, Color::Black() });
                Renderer::DrawDebugRectangle(Rectangle(start - Vector2(2, 2), Vector2(4, 4)), c);
            }
        }
    }
#endif

    if (state == PathFollowerState::Stopped)
    {
        return;
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
        else if (flowField != nullptr)
        {
            flowField->target = target->Center();
        }
    }
    else
    {
        state = PathFollowerState::Stopped;
        flowField = nullptr;
        Log("Field set to null because stopped\n");
    }
}

void PathFollowerComponent::FollowFlowField()
{
    auto scene = GetScene();

    if (flowField != nullptr)
    {
        Vector2 velocity;

        if (false)
        {
            velocity = (flowField->target - owner->Center()).Normalize() * 200;
        }
        else
        {
            currentLayer = scene->isometricSettings.terrain[ToPathfinderPerspective(owner->Center())].height;

            if ((intermediateTarget - owner->Center()).Length() < 1)
            {
                // TODO: this is kind of hacky, but we only change the current layer once we arrive at the intermediate target.
                // The assumption is that we don't change layers when bee-lining.

                bool firstMove = true;
                do
                {
                    Vector2 pathFinderPerspective = ToPathfinderPerspective(intermediateTarget);

                    auto dir = flowField->grid[pathFinderPerspective].dir;

                    if (dir == Vector2(0, 0))
                    {
                        intermediateTarget = currentTarget;
                        break;
                    }

                    auto nextTile = pathFinderPerspective.Floor().AsVectorOfType<float>() + dir;
                    auto nextTarget = scene->isometricSettings.TileToWorld(nextTile + Vector2(0.5));

                    Vector2 checkPoints[5];
                    scene->isometricSettings.GetTileBoundaries(nextTile, checkPoints);
                    checkPoints[4] = nextTarget;

                    bool fail = false;

//                    checkPoints[0] -= Vector2(0, 1);
//                    checkPoints[1] += Vector2(1, 0);
//                    checkPoints[2] += Vector2(0, 1);
//                    checkPoints[3] -= Vector2(1, 0);

                    for (auto point : checkPoints)
                    {
                        RaycastResult result;
                        if (scene->Raycast(owner->Center(), point, result, true, [=](const ColliderHandle& handle)
                        {
                            return handle.OwningEntity()->Is<TilemapEntity>()
                                || !handle.IsTrigger();
                        }))
                        {
                            if (!firstMove)
                            {
                                fail = true;
                                break;
                            }
                        }
                    }

                    if (fail)
                    {
                        break;
                    }

                    firstMove = false;
                    intermediateTarget = nextTarget;

                    if (nextTile == scene->isometricSettings.WorldToTile(currentTarget))
                    {
                        break;
                    }
                } while(true);
            }

            velocity = (intermediateTarget - owner->Center()).Normalize() * speed;
        }

        float dist = (flowField->target - owner->Center()).Length();
        if (dist > 20)
        {
            velocity = rigidBody->GetVelocity().SmoothDamp(
                velocity,
                acceleration,
                0.05,
                Scene::PhysicsDeltaTime);
        }

        if (dist <= 1)
        {
            velocity = { 0, 0 };
            acceleration = { 0, 0 };
            flowField = nullptr;
        }

        rigidBody->SetVelocity(velocity);
        //Renderer::DrawDebugLine({ owner->Center(), owner->Center() + velocity, useBeeLine ? Color::Red() : Color::Green() });
    }
}

void PathFollowerComponent::UpdateFlowField(Vector2 newTarget)
{
    currentTarget = newTarget;
    intermediateTarget = owner->Center();

    auto targetPathFinderPerspective = ToPathfinderPerspective(newTarget);
    auto centerPathFinderPerspective = ToPathfinderPerspective(owner->Center());
    GetScene()->GetService<PathFinderService>()->RequestFlowField(centerPathFinderPerspective, targetPathFinderPerspective, owner);
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
}

void PathFollowerComponent::ReceiveEvent(const IEntityEvent& ev)
{
    if (auto flowFieldReady = ev.Is<FlowFieldReadyEvent>())
    {
        if (state == PathFollowerState::Stopped) return;
        if (!flowFieldReady->result->grid[ToPathfinderPerspective(currentTarget)].alreadyVisited)
        {
            // Failed to find a path
            return;
        }

        flowField = flowFieldReady->result;
        flowField->target = currentTarget;
        acceleration = { 0, 0 };
    }
}

void PathFollowerComponent::FollowEntity(Entity* entity, float minDistance)
{
    state = PathFollowerState::FollowingEntity;
    UpdateFlowField(entity->Center());
    entityToFollow = entity;
    updateTargetTimer = 2;
}

Vector2 PathFollowerComponent::ToPathfinderPerspective(Vector2 position)
{
    auto scene = GetScene();
    if (scene->perspective == ScenePerspective::Orothgraphic) return position / Vector2(32, 32);    // TODO: don't hardcode
    else if (scene->perspective == ScenePerspective::Isometric) return scene->isometricSettings.WorldToTile(position);

    return position;
}