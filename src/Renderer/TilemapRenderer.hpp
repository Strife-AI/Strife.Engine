#pragma once

#include <vector>


#include "Renderer.hpp"
#include "Scene/MapSegment.hpp"

class Camera;
class SpriteBatcher;
class Renderer;

class TilemapLayerRenderer
{
public:
    TilemapLayerRenderer(const MapLayer* layer)
        : _layer(layer)
    {
    }
    void SetOffset(Vector2 offset)
    {
        _offset = offset;
    }

    Vector2 Offset() const
    {
        return _offset;
    }

    const MapLayer* GetMapLayer() const { return _layer; };

private:
    const MapLayer* _layer;
    Vector2 _offset;
};

class TilemapRenderer
{
public:
    void SetMapSegment(const MapSegment* mapSegment, Scene* scene);
    void Render(Renderer* renderer) const;
    void SetOffset(Vector2 offset);

private:
    std::vector<TilemapLayerRenderer> _layers;
    Scene* _scene;
};
