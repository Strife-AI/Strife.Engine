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
    void OnAdded(const EntityDictionary& properties) override;
    void OnDestroyed() override;
    const MapSegment* _mapSegment = nullptr;

    TilemapRenderer _renderer;
    AmbientLight light;
};
