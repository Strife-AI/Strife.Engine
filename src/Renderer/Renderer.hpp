#pragma once

#include <Math/Rectangle.hpp>
#include <Renderer/GL/LineRenderer.hpp>
#include <Renderer/GL/SpriteBatcher.hpp>
#include "Math/Vector2.hpp"
#include "SpriteFont.hpp"
#include "Color.hpp"

class NineSlice;
class Sprite;
class Scene;
class Camera;
struct ICustomTransparencyRenderer;
struct SDL_Renderer;

struct FontSettings
{
    FontSettings()
        : spriteFont(nullptr),
        scale(1)
    {

    }

    FontSettings(Resource<SpriteFont> spriteFont_, float scale_ = 1)
            : spriteFont(spriteFont_),
              scale(scale_)
    {
    }

    Resource<SpriteFont> spriteFont;
    float scale;
};

struct DebugLine
{
    DebugLine(Vector2 start_, Vector2 end_, Color color_)
        : start(start_),
        end(end_),
        color(color_)
    {
        
    }

    Vector2 start;
    Vector2 end;
    Color color;
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
    SDL_Renderer* SdlRenderer() const { return _sdlRenderer; }

    void RenderSprite(const Sprite* sprite, Vector2 position, float depth, Vector2 scale = Vector2(1, 1), float angle = 0, bool flipHorizontal = false, Color blendColor = Color());
    void RenderSpriteRepeated(const Sprite* sprite, const Rectangle& bounds, float depth, Vector2 center, float angle = 0);

    void RenderNineSlice(const NineSlice* nineSlice, const Rectangle& bounds, float depth);
    void RenderThreeSlice(const Sprite* sprite, float cornerXSize, const Rectangle& bounds, float depth, float angle);
    void RenderString(const FontSettings& fontSettings, const char* str, Vector2 topLeft, float depth);
    void RenderRectangle(const Rectangle& rect, Color color, float depth, float angle = 0);

    void RenderLine(const Vector2 start, const Vector2 end, const Color color, float depth);
    void RenderCircleOutline(Vector2 center, float radius, Color color, float depth, float subdivide = 32);
    void RenderCircle(Vector2 center, float radius, Color color, float depth, float subdivide = 32);
    void RenderRectangleOutline(const Rectangle& rect, Color color, float depth);

    void RenderCustomTransparency(const ICustomTransparencyRenderer* renderer, float depth);

    void BeginRender(Scene* scene, Camera* camera, Vector2 renderOffset, float deltaTime, float absoluteTime);
    void DoRendering();
    void RenderSpriteBatch();

    Camera* GetCamera() const { return _camera; }
    void SetRenderOffset(Vector2 renderOffset) { _renderOffset = renderOffset; }
    Vector2 RenderOffset() const { return _renderOffset; }

    SpriteBatcher* GetSpriteBatcher() { return &_spriteBatcher; }
    LightManager* GetLightManager() { return &_lightManager; }

    static void DrawDebugLine(const DebugLine& line)
    {
        _debugLines.push_back(line);
    }

    static void DrawDebugRectangleOutline(const Rectangle& rect, Color color);
    static void DrawDebugRectangle(const Rectangle& rect, Color color);

private:
    void InitializeLineRenderer();
    void InitializeSpriteBatcher();
    void ResizeScreen(int w, int h);

    struct SDL_Renderer* _sdlRenderer;
    float _deltaTime;
    float _absoluteTime;

    LineRenderer _lineRenderer;
    Shader _lineShader;

    SpriteBatcher _spriteBatcher;
    Shader _spriteShader;

    LightManager _lightManager;

    Camera* _camera;
    Vector2 _renderOffset;
    Scene* _scene;

    std::unique_ptr<FrameBuffer> _deferredLightingColorBuffer;

    static std::vector<DebugLine> _debugLines;
    static std::vector<DebugRectangle> _debugRectangles;
};
