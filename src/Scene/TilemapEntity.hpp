#pragma once
#include "Scene/MapSegment.hpp"
#include "Scene/Entity.hpp"
#include "Renderer/TilemapRenderer.hpp"
#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(TilemapEntity, "tilemap")
{
    void SetMapSegment(const MapSegment& mapSegment);
    const MapSegment* GetMapSegment() const { return _mapSegment; }

	void Render(Renderer* renderer) override;

private:
    void OnAdded() override;
    void OnDestroyed() override;
    const MapSegment* _mapSegment = nullptr;

    TilemapRenderer _renderer;
    AmbientLight light;
};

DEFINE_ENTITY(WalkableTerrainEntity0, "walkable-terrain-0")
{
    void OnAdded() override;
    RigidBodyComponent* rigidBody;
};

DEFINE_ENTITY(WalkableTerrainEntity1, "walkable-terrain-1")
{
    void OnAdded() override;
    RigidBodyComponent* rigidBody;
};

DEFINE_ENTITY(WalkableTerrainEntity2, "walkable-terrain-2")
{
    void OnAdded() override;
    RigidBodyComponent* rigidBody;
};

DEFINE_ENTITY(RampEntity, "ramp")
{
    void OnAdded() override;
    RigidBodyComponent* rigidBody;
};