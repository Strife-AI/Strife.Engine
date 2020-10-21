#pragma once


#include <memory>
#include <Renderer/Sprite.hpp>
#include <unordered_map>
#include <Renderer/RenderVertex.hpp>
#include <Renderer/Color.hpp>

#include "Lighting.hpp"
#include "Shader.hpp"
#include "gsl/span"
#include "Memory/FixedSizeVector.hpp"
#include "Renderer/Texture.hpp"

class SpriteBatcher;

struct Polygon
{
    Polygon(gsl::span<RenderVertex> vertices_)
        : vertices(vertices_)
    {
        
    }

    void RotateXY(Vector2 center, float angle)
    {
        float cx = cos(angle);
        float cy = sin(angle);

        Vector2 i(cx, -cy);
        Vector2 j(cy, cx);

        for (auto& vertex : vertices)
        {
            auto relative = vertex.position.XY() - center;

            float x = i.Dot(relative);
            float y = j.Dot(relative);

            vertex.position = { x + center.x, y + center.y, vertex.position.z };
        }
    }

    Rectangle Bounds() const
    {
        Rectangle bounds(vertices[0].position.XY(), Vector2(0, 0));

        for(int i = 1; i < vertices.size(); ++i)
        {
            auto v = vertices[i].position.XY();
            bounds.topLeft = bounds.topLeft.Min(v);
            bounds.bottomRight = bounds.bottomRight.Max(v);
        }

        return bounds;
    }

    gsl::span<RenderVertex> vertices;
};

struct ICustomTransparencyRenderer
{
    virtual void Render(SpriteBatcher* batcher, Camera* camera, float depth) const = 0;
};

struct TransparentObject
{
    TransparentObject() = default;
    TransparentObject(int firstVertex_, float z_, Texture* texture_)
        : firstVertex(firstVertex_),
        texture(texture_),
        z(z_)
    {

    }

    TransparentObject(const ICustomTransparencyRenderer* renderer, float z_)
        : z(z_),
        customRenderer(renderer)
    {
        
    }

    bool operator<(const TransparentObject& rhs) const
    {
        return z > rhs.z;
    }

    int firstVertex;
    Texture* texture = nullptr;
    float z;
    const ICustomTransparencyRenderer* customRenderer = nullptr;
};

class SpriteBatcher
{
public:
    void Initialize(Shader* shader);
    void RenderSprite(const Sprite& sprite, const Vector3 position, const Vector2 scale, float angle, bool flipHorizontal, Color blendColor);
    void RenderSolidColor(Color color, const Vector3 position, const Vector2 size, Texture* textureOverride = nullptr, float angle = 0);
    void RenderSolidPolygon(Vector2* vertices, int vertexCount, float z, Color color);
    void RenderCustomTransparency(const ICustomTransparencyRenderer* renderer, float z);

    void BeginRender(Camera* camera);

    void Render();
    void RenderPolygon(Polygon& polygon, Texture* texture);

private:
    static constexpr int MaxVerticesPerBatch = 65536 * 2;
    static constexpr int MaxElementsPerBatch = MaxVerticesPerBatch * 4;

    static constexpr int VertexLocation = 0;
    static constexpr int TextureCoordLocation = 1;
    static constexpr int ColorLocation = 2;

    int UnformArrayLocation(const char* arrayName, const char* property, int index) const;

    Shader* _shader;

    unsigned int vao;
    unsigned int spriteVerticesVbo;
    unsigned int ebo;

    int _viewMatrixLocation;
    int _lightPositionLocation;
    int _botLightLocation;
    int _useLightingLocation;
    int _lightDistanceLocation;
    int _totalPointLightsLocation;

    FixedSizeVector<Texture*, 512> _texturesRenderedThisFrame;
    unsigned int _currentFrame = 0;

    static constexpr int MaxTransparentTriangles = 8192;
    FixedSizeVector<TransparentObject, MaxTransparentTriangles> _transparentObjects;
    FixedSizeVector<RenderVertex, MaxElementsPerBatch> _vertices;

    std::unique_ptr<Texture> _solidColor;
    std::unique_ptr<Texture> _transparencyTexture;

    Camera* _camera;
    bool _ignoreTransparency = false;
    int _vertexResetSize = 0;
};

