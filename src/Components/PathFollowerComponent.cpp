#include "PathFollowerComponent.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Renderer.hpp"

void PathFollowerComponent::FixedUpdate(float deltaTime)
{
#if 1
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

        Vector2 points[4];
        owner->Bounds().GetPoints(points);

        bool useBeeLine = true;
        for (auto p : points)
        {
            RaycastResult result;
            if (scene->Raycast(p, flowField->target, result, false, [=](auto collider)
            {
                return collider.OwningEntity() !=
                       owner;
            })
                && (state != PathFollowerState::FollowingEntity ||
                    result.handle.OwningEntity() != entityToFollow.GetValueOrNull()))
            {
                useBeeLine = false;
                break;
            }
        }

        useBeeLine = false;

        if (useBeeLine)
        {
            velocity = (flowField->target - owner->Center()).Normalize() * 200;
        }
        else
        {
            if ((intermediateTarget - owner->Center()).Length() < 1)
            {
                auto pathFinderPerspective = ToPathfinderPerspective(owner->Center());

                auto nextTile = pathFinderPerspective.Floor().AsVectorOfType<float>() + flowField->grid[pathFinderPerspective].dir;

                intermediateTarget = scene->isometricSettings.TileToWorld(nextTile + Vector2(0.5));
            }

            velocity = (intermediateTarget - owner->Center()).Normalize() * speed;
        }

        float dist = (flowField->target - owner->Center()).Length();
        if (dist > 20 && false)
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
            Log("Field set to null\n");
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

