#pragma once

#include "Scene/EntityComponent.hpp"
#include "Physics/PathFinding.hpp"

enum class PathFollowerState
{
	Stopped,
	MovingToTarget,
	FollowingEntity
};

DEFINE_COMPONENT(PathFollowerComponent)
{
	PathFollowerComponent(RigidBodyComponent* rigidBody)
		: rigidBody(rigidBody)
	{

	}

	void SetTarget(Vector2 position);
	void Stop(bool loseVelocity);
	void FollowEntity(Entity* entity, float minDistance);

	void UpdateFlowField(Vector2 newTarget);
	void FixedUpdate(float deltaTime) override;
	void ReceiveEvent(const IEntityEvent& ev);
	void FollowFlowField();
	void UpdateFollowTarget(float deltaTime, Scene* scene);

	RigidBodyComponent* rigidBody;
	std::shared_ptr<FlowField> flowField;
	Vector2 acceleration;
	bool arrived = true;
	PathFollowerState state = PathFollowerState::Stopped;
	EntityReference<Entity> entityToFollow;
	float updateTargetTimer = 0;
	float speed = 200;
};