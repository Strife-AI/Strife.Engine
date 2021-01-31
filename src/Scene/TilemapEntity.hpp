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

DEFINE_ENTITY(WalkableTerrainEntity, "walkable-terrain")
{
    void OnAdded() override;

    int height = 0;
    RigidBodyComponent* rigidBody;
};
