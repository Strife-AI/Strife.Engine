#pragma once

#include "Scene/EntityComponent.hpp"
#include "Physics/PathFinding.hpp"

enum class PathFollowerState
{
    Stopped,
    MovingToTarget,
    FollowingEntity
};

enum class PathFollowerPathType
{
    None,
    FlowField,
    Beeline,
};

struct PathFollowerPath
{
    std::vector<Vector2> path;
    int nextPathIndex = 0;

    Vector2 target;
    PathFollowerPathType type;
    std::shared_ptr<FlowField> flowField;
};

DEFINE_COMPONENT(PathFollowerComponent)
{
    PathFollowerComponent(RigidBodyComponent* rigidBody)
        : rigidBody(rigidBody)
    {

    }

    void OnAdded() override;
    void OnRemoved() override;
    void SetTarget(Vector2 position);
    void Stop(bool loseVelocity);
    void FollowEntity(Entity* entity, float minDistance);

    void UpdateFlowField(Vector2 newTarget);
    void FixedUpdate(float deltaTime) override;
    void ReceiveEvent(const IEntityEvent& ev);
    void FollowFlowField();
    void UpdateFollowTarget(float deltaTime, Scene* scene);
    bool CanBeeline(Vector2 from, Vector2 to);
    void Update(float deltaTime);

    Vector2 ToPathfinderPerspective(Vector2 position);

    RigidBodyComponent* rigidBody;
    PathFollowerPath currentPath;
    Vector2 acceleration;
    bool arrived = true;
    PathFollowerState state = PathFollowerState::Stopped;
    EntityReference<Entity> entityToFollow;
    float updateTargetTimer = 0;
    float speed = 200;
    Vector2 intermediateTarget;
};