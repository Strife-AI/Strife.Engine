#include "PathFollowerComponent.hpp"
#include "Components/RigidBodyComponent.hpp"

void PathFollowerComponent::FixedUpdate(float deltaTime)
{
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

        if (useBeeLine)
        {
            velocity = (flowField->target - owner->Center()).Normalize() * 200;
        }
        else
        {
            velocity = flowField->GetFilteredFlowDirection(owner->Center() - Vector2(16, 16)) * speed;
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
    GetScene()->GetService<PathFinderService>()->RequestFlowField(owner->Center(), newTarget, owner);
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
