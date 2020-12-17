#pragma once
#include "Scene/MapSegment.hpp"
#include "Scene/Entity.hpp"
#include "Renderer/TilemapRenderer.hpp"
#include "Scene/BaseEntity.hpp"

DEFINE_ENTITY(TilemapEntity, "tilemap"), IRenderable
{
    void SetMapSegment(const MapSegment& mapSegment);
    const MapSegment* GetMapSegment() const { return _mapSegment; }

private:
    void OnAdded(const EntityDictionary& properties) override;
    void OnDestroyed() override;
    void Render(Renderer* renderer) override;
    const MapSegment* _mapSegment = nullptr;

    TilemapRenderer _renderer;
    AmbientLight light;
};
