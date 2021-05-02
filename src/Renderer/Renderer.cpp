#include <Resource/ShaderResource.hpp>
#include "Renderer.hpp"
#include "Camera.hpp"
#include "Engine.hpp"
#include "NineSlice.hpp"
#include "SdlManager.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneManager.hpp"
#include "GL/gl3w.h"
#include "SpriteEffect.hpp"
#include "Stage/RenderPipeline.hpp"

static ConsoleVar<bool> g_useLighting("light", true);

std::vector<DebugLine> Renderer::_debugLines;
std::vector<DebugRectangle> Renderer::_debugRectangles;

std::unique_ptr<SpriteEffect> spriteEffect;

Renderer::Renderer()
{
    auto spriteShader = GetResource<ShaderResource>("sprite-shader");
    spriteEffect = std::make_unique<SpriteEffect>(&spriteShader->Get());

    InitializeLineRenderer();

    _solidColor = std::make_unique<Texture>(Color(0, 0, 0, 255), 1, 1);
}

Vector2 RotateXY(Vector2 center, Vector2 vertex, float angle)
{
    if (angle == 0) return vertex;

    float cx = cos(angle);
    float cy = sin(angle);

    Vector2 i(cx, -cy);
    Vector2 j(cy, cx);

    auto relative = vertex - center;

    float x = i.Dot(relative);
    float y = j.Dot(relative);

    return { x + center.x, y + center.y };
}

static void ConstructSpriteQuad(
    const Rectangle& bounds,
    const Rectangle& textureCoord,
    float depth,
    float angle,
    Color blendColor,
    bool flipHorizontal,
    RenderVertex result[4])
{
    Vector2 corners[4];
    bounds.GetPoints(corners);

    if (flipHorizontal)
    {
        std::swap(corners[0], corners[1]);
        std::swap(corners[2], corners[3]);
    }

    Vector2 textureCoords[4];
    textureCoord.GetPoints(textureCoords);

    auto center = bounds.GetCenter();

    for (int i = 0; i < 4; ++i)
    {
        result[i].position = Vector3(RotateXY(center, corners[i], angle), depth);
        result[i].color = blendColor.ToVector4();
        result[i].textureCoord = textureCoords[i];
    }
}

void Renderer::RenderSprite(const Sprite* sprite, const Vector2 position, float depth, Vector2 scale, float angle, bool flipHorizontal, Color blendColor)
{
    Rectangle bounds(
        position + _renderOffset,
        sprite->Bounds().Size() * scale);

    RenderVertex v[4];
    ConstructSpriteQuad(bounds, sprite->UVBounds(), depth, angle, blendColor, flipHorizontal, v);
    spriteEffect->RenderPolygon(v, sprite->GetTexture());
}

void Renderer::RenderString(const FontSettings& fontSettings, const char* str, Vector2 topLeft, float depth)
{
    if (fontSettings.spriteFont == nullptr)
    {
        return;
    }

    auto characterSize = fontSettings.spriteFont->GetFont()->CharacterDimension(fontSettings.scale);
    Vector2 position = topLeft;

    while (*str != '\0')
    {
        if (*str == '\n')
        {
            position.x = topLeft.x;
            position.y += characterSize.y;
        }
        else
        {
            Sprite characterSprite;
            fontSettings.spriteFont->GetFont()->GetCharacter((unsigned char)*str, &characterSprite);

            RenderSprite(
                &characterSprite,
                position,
                depth,
                Vector2(fontSettings.scale, fontSettings.scale));

            position.x += characterSize.x;
        }

        ++str;
    }
}

void Renderer::RenderRectangle(const Rectangle& rect, Color color, float depth, float angle)
{
//    _spriteBatcher.RenderSolidColor(
//        color,
//        Vector3(
//            rect.topLeft.x + _renderOffset.x,
//            rect.topLeft.y + _renderOffset.y,
//            depth),
//        rect.Size(),
//        nullptr,
//        angle);
}

void Renderer::RenderLine(Vector2 start, Vector2 end, Color color, float depth)
{
    _lineRenderer.DrawLine(
        Vector3(start.x + _renderOffset.x, start.y + _renderOffset.y, depth),
        Vector3(end.x + _renderOffset.x, end.y + _renderOffset.y, depth),
        color);
}

void Renderer::RenderCircleOutline(Vector2 center, float radius, Color color, float depth, float subdivide)
{
    float dAngle = 2 * 3.14159 / subdivide;
    Vector2 last = center + Vector2(radius, 0);

    for (int i = 1; i <= subdivide; ++i)
    {
        float angle = dAngle * i;
        Vector2 point = center + Vector2(cos(angle), sin(angle)) * radius;

        RenderLine(point, last, color, depth);
        last = point;
    }
}

void Renderer::RenderCircle(Vector2 center, float radius, Color color, float depth, int subdivide)
{
    float dAngle = 2.0f * 3.14159f / subdivide;

    RenderVertex v[128];

    for (int i = 0; i < subdivide; ++i)
    {
        float angle = dAngle * i;
        v[i].position = Vector3(center + Vector2(cos(angle), sin(angle)) * radius, depth);
        v[i].color = color.ToVector4();
    }

    spriteEffect->RenderPolygon(gsl::span<RenderVertex>(v, subdivide), SolidColorTexture());
}

void Renderer::DoRendering()
{
    auto screenSize = _camera->ScreenSize();
    glViewport(0, 0, screenSize.x, screenSize.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Only do debug rendering on the client
    if (!_scene->isServer)
    {
        for (auto& rect : _debugRectangles)
        {
            RenderRectangle(rect.rect, rect.color, -1, 0);
        }

        _debugRectangles.clear();

        int aliveIndex = 0;
        for (int i = 0; i < _debugLines.size(); ++i)
        {
            auto& line = _debugLines[i];
            line.time -= _deltaTime;
            RenderLine(line.start, line.end, line.color, -1);

            if (line.time > 0)
            {
                _debugLines[aliveIndex++] = line;
            }
        }

        _debugLines.resize(aliveIndex);
    }

    _scene->RenderEntities(this);
    _rendererState.SetActiveEffect(nullptr);

    _lineRenderer.Render(_camera->ViewMatrix());
}

void Renderer::RenderSpriteBatch()
{
    glEnable(GL_DEPTH_TEST);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    _lineRenderer.Render(_camera->ViewMatrix());
}

void Renderer::BeginRender(Scene* scene, float deltaTime, float absoluteTime)
{
    _scene = scene;
    _deltaTime = deltaTime;
    _absoluteTime = absoluteTime;
}

void Renderer::DrawDebugRectangleOutline(const Rectangle& rect, Color color)
{
    Vector2 points[4];
    rect.GetPoints(points);

    for (int i = 0; i < 4; ++i)
    {
        _debugLines.emplace_back(points[i], points[(i + 1) % 4], color);
    }
}

void Renderer::DrawDebugRectangle(const Rectangle& rect, Color color)
{
    _debugRectangles.emplace_back(rect, color);
}

void Renderer::InitializeLineRenderer()
{
    const char* vertexShader = R"(
#version 330 core

uniform mat4x4 view;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 vertexColor;

out vec4 color;

void main()
{
    gl_Position = view * vec4(aPos, 1.0);
    color = vertexColor;
})";

    const char* fragmentShader = R"(
#version 330 core

in vec4 color;

out vec4 FragColor;

void main()
{
    FragColor = color;
})";

    _lineShader.Compile(vertexShader, fragmentShader);
    _lineRenderer.Initialize(&_lineShader);
}

void Renderer::RenderRectangleOutline(const Rectangle& rect, Color color, float depth)
{
    Vector2 topRight = Vector2(rect.bottomRight.x, rect.topLeft.y);
    Vector2 bottomLeft = Vector2(rect.topLeft.x, rect.bottomRight.y);

    RenderLine(rect.topLeft, topRight, color, depth);
    RenderLine(bottomLeft, rect.bottomRight, color, depth);
    RenderLine(rect.topLeft, bottomLeft, color, depth);
    RenderLine(topRight, rect.bottomRight, color, depth);
}

void Renderer::RenderPolygon(gsl::span<RenderVertex> vertices, Texture* texture)
{
    spriteEffect->RenderPolygon(vertices, texture);
}

void Renderer::RenderSolidColorPolygon(gsl::span<Vector2> vertices, Color color, float depth)
{
    RenderVertex rv[128];
    for (int i = 0; i < vertices.size(); ++i)
    {
        rv[i].position = Vector3(vertices[i], depth);
        rv[i].color = color.ToVector4();
    }

    spriteEffect->RenderPolygon(gsl::span<RenderVertex>(rv, vertices.size()), SolidColorTexture());
}

void Renderer::RenderCamera(Camera* camera, const std::function<void(RenderPipelineState& state)>& executePipeline)
{
    Effect::renderer = &_rendererState;
    _rendererState = RendererState();
    _rendererState.camera = camera;

    _camera = camera;
    _camera->RebuildViewMatrix();

    RenderPipelineState state;
    state.renderer = this;
    state.absoluteTime = _absoluteTime;
    state.deltaTime = _deltaTime;
    state.scene = _scene;
    state.rendererState = &_rendererState;

    executePipeline(state);
}
