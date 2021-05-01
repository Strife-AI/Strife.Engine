#pragma once

#include "Math/Rectangle.hpp"
#include "Renderer/GL/LineRenderer.hpp"
#include "Lighting.hpp"
#include "Math/Vector2.hpp"
#include "SpriteFont.hpp"
#include "Color.hpp"
#include "Resource/SpriteFontResource.hpp"

class NineSlice;
class Sprite;
class Scene;
class Camera;

struct FontSettings
{
    FontSettings()
        : spriteFont(nullptr),
        scale(1)
    {

    }

    FontSettings(SpriteFontResource* spriteFont_, float scale_ = 1)
            : spriteFont(spriteFont_),
              scale(scale_)
    {
    }

    SpriteFontResource* spriteFont;
    float scale;
};

struct DebugLine
{
    DebugLine(){ }

    DebugLine(Vector2 start_, Vector2 end_, Color color_, float time = 0)
        : start(start_),
        end(end_),
        color(color_),
        time(time)
    {
        
    }

    Vector2 start;
    Vector2 end;
    Color color;
    float time;
};

struct DebugRectangle
{
    DebugRectangle(const Rectangle& rect_, Color color_)
        : rect(rect_),
        color(color_)
    {
        
    }

    Rectangle rect;
    Color color;
};

class Renderer
{
public:
    Renderer();

    float DeltaTime() const { return _deltaTime; }
    float CurrentTime() const { return _absoluteTime; }

    void RenderSprite(const Sprite* sprite, Vector2 position, float depth, Vector2 scale = Vector2(1, 1), float angle = 0, bool flipHorizontal = false, Color blendColor = Color());

    void RenderPolygon(gsl::span<RenderVertex> vertices, Texture* texture);
    void RenderSolidColorPolygon(gsl::span<Vector2> vertices, Color color, float depth);

    void RenderString(const FontSettings& fontSettings, const char* str, Vector2 topLeft, float depth);
    void RenderRectangle(const Rectangle& rect, Color color, float depth, float angle = 0);

    void RenderLine(Vector2 start, Vector2 end, Color color, float depth);
    void RenderCircleOutline(Vector2 center, float radius, Color color, float depth, float subdivide = 32);
    void RenderCircle(Vector2 center, float radius, Color color, float depth, int subdivide = 32);
    void RenderRectangleOutline(const Rectangle& rect, Color color, float depth);

    void BeginRender(Scene* scene, Camera* camera, Vector2 renderOffset, float deltaTime, float absoluteTime);
    void DoRendering();
    void RenderSpriteBatch();

    Camera* GetCamera() const { return _camera; }
    void SetRenderOffset(Vector2 renderOffset) { _renderOffset = renderOffset; }
    Vector2 RenderOffset() const { return _renderOffset; }

    Texture* SolidColorTexture() const { return _solidColor.get(); }

    LightManager* GetLightManager() { return &_lightManager; }

    static void DrawDebugLine(const DebugLine& line)
    {
        _debugLines.push_back(line);
    }

    static void DrawDebugRectangleOutline(const Rectangle& rect, Color color);
    static void DrawDebugRectangle(const Rectangle& rect, Color color);

private:
    void InitializeLineRenderer();

    float _deltaTime;
    float _absoluteTime;

    LineRenderer _lineRenderer;
    Shader _lineShader;

    LightManager _lightManager;

    Camera* _camera;
    Vector2 _renderOffset;
    Scene* _scene;
    RendererState _rendererState;

    std::unique_ptr<Texture> _solidColor;

    static std::vector<DebugLine> _debugLines;
    static std::vector<DebugRectangle> _debugRectangles;
};
