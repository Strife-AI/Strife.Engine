#pragma once

#include <vector>


#include "Renderer.hpp"
#include "Scene/MapSegment.hpp"

class Camera;
class SpriteBatcher;
class Renderer;

class TilemapLayerRenderer : public ICustomTransparencyRenderer
{
public:
    TilemapLayerRenderer(const MapLayer* layer)
        : _layer(layer)
    {
    }

    void Render(SpriteBatcher* batcher, Camera* camera, float depth) const override;
    void SetOffset(Vector2 offset)
    {
        _offset = offset;
    }

    const MapLayer* GetMapLayer() const { return _layer; };

private:
    const MapLayer* _layer;
    Vector2 _offset;
};

class TilemapRenderer
{
public:
    void SetMapSegment(const MapSegment* mapSegment);
    void Render(Renderer* renderer) const;

    void SetOffset(Vector2 offset);

private:
    std::vector<TilemapLayerRenderer> _layers;
};
