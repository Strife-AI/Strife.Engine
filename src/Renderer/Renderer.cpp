#include "Renderer.hpp"
#include "Camera.hpp"
#include "Engine.hpp"
#include "NineSlice.hpp"
#include "SdlManager.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneManager.hpp"
#include "GL/gl3w.h"

static ConsoleVar<bool> g_useLighting("light", true);

std::vector<DebugLine> Renderer::_debugLines;
std::vector<DebugRectangle> Renderer::_debugRectangles;

Renderer::Renderer()
{
    InitializeLineRenderer();
    InitializeSpriteBatcher();

    WindowSizeChangedEvent::AddHandler([=](const WindowSizeChangedEvent& ev)
    {
        ResizeScreen(ev.width, ev.height);
    });
}

bool draw = false;


void Renderer::RenderSprite(const Sprite* sprite, const Vector2 position, float depth, Vector2 scale, float angle, bool flipHorizontal, Color blendColor)
{
    if (draw)
        RenderRectangleOutline(sprite->Bounds().AddPosition(position), Color::Red(), -1);
    _spriteBatcher.RenderSprite(
        *sprite,
        Vector3(
            position.x + _renderOffset.x,
            position.y + _renderOffset.y,
            depth),
        scale,
        angle,
        flipHorizontal,
        blendColor);
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

void Renderer::RenderSpriteRepeated(const Sprite* sprite, const Rectangle& bounds, float depth, Vector2 center, float angle)
{
    float spriteW = sprite->Bounds().Width();
    float spriteH = sprite->Bounds().Height();
    Vector2 tileSize(spriteW, spriteH);

    int w = Max(1.0f, ceil(bounds.Width() / spriteW));
    int h = Max(1.0f, ceil(bounds.Height() / spriteH));

    for (int i = 0; i < h; ++i)
    {
        for (int j = 0; j < w; ++j)
        {
            auto tileTopLeft = bounds.TopLeft() + tileSize * Vector2(j, i);
            if (i != h - 1 && j != w - 1)
            {
                auto position = RotateXY(center, tileTopLeft + tileSize / 2, angle) - tileSize / 2;
                RenderSprite(sprite, position, depth, Vector2(1, 1), angle);
            }
            else
            {
                auto tileBottomRight = (tileTopLeft + tileSize).Min(bounds.BottomRight());
                auto subTileSize = tileBottomRight - tileTopLeft;
                auto textureScale = subTileSize / tileSize;

                Rectangle textureBounds(
                    sprite->UVBounds().TopLeft(),
                    sprite->UVBounds().Size() * textureScale);

                textureBounds = textureBounds.Scale(sprite->GetTexture()->Size());

                Sprite subSprite(
                    sprite->GetTexture(),
                    Rectangle(tileTopLeft, subTileSize),
                    textureBounds);

                auto position = RotateXY(center, tileTopLeft + subTileSize / 2, angle) - subTileSize / 2;
                RenderSprite(&subSprite, position, depth, Vector2(1, 1), angle);
            }
        }
    }
}

void Renderer::RenderNineSlice(const NineSlice* nineSlice, const Rectangle& bounds, float depth)
{
#if false
    auto texture = nineSlice->GetSprite()->GetTexture();
    auto cornerSize = nineSlice->CornerSize();
    auto textureCornerSize = cornerSize;

    auto uvBounds = nineSlice->GetSprite()->UVBounds().Scale(texture->Size());
    auto textureSize = uvBounds.Size();
    auto textureTopLeft = uvBounds.TopLeft();

    if (cornerSize.x * 2 > bounds.Width())
    {
        cornerSize.x = bounds.Width() / 2;
    }

    Vector2 positions[4] = { Vector2::Zero(), cornerSize, bounds.Size() - cornerSize, bounds.Size() };
    Vector2 texCoords[4] =
    {
        textureTopLeft,
        textureTopLeft + textureCornerSize + Vector2(8, 8),
        textureTopLeft + textureSize - textureCornerSize,
        textureTopLeft + textureSize
    };

    draw = false;

    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            Rectangle spriteBounds(
                Vector2(positions[x].x, positions[y].y),
                Vector2(positions[x + 1].x - positions[x].x, positions[y + 1].y - positions[y].y));

            if(x == 1 && y == 1)
            {

                Sprite centerSprite(
                    nineSlice->GetCenterSprite()->GetTexture(),
                    Rectangle(Vector2(0, 0), spriteBounds.Size()),
                    Rectangle(Vector2(0, 0), spriteBounds.Size()));

                RenderSprite(&centerSprite, bounds.TopLeft() + spriteBounds.TopLeft(), depth);
            }
            else
            {
                Rectangle textureBounds(
                    Vector2(texCoords[x].x, texCoords[y].y),
                    Vector2(texCoords[x + 1].x - texCoords[x].x, texCoords[y + 1].y - texCoords[y].y));

                if (x != 2) textureBounds.bottomRight.x -= 8;
                if (y != 2) textureBounds.bottomRight.y -= 8;

                Sprite sprite(texture, spriteBounds, textureBounds);
                RenderSprite(&sprite, bounds.TopLeft() + spriteBounds.TopLeft(), depth, Vector2(1, 1));
            }
        }
    }
#endif
}

void Renderer::RenderThreeSlice(const Sprite* sprite, float cornerXSize, const Rectangle& bounds, float depth, float angle)
{
    auto texture = sprite->GetTexture();

    auto uvBounds = sprite->UVBounds().Scale(texture->Size());
    auto textureSize = uvBounds.Size();
    auto textureTopLeft = uvBounds.TopLeft();

    float positions[4] = { 0, cornerXSize, bounds.Size().x - cornerXSize, bounds.Size().x };

    float texCoords[4] =
    {
        textureTopLeft.x,
        textureTopLeft.x + cornerXSize,
        textureTopLeft.x + textureSize.x - cornerXSize,
        textureTopLeft.x + textureSize.x
    };

    for (int x = 0; x < 3; ++x)
    {
        Rectangle spriteBounds(
            Vector2(positions[x], 0),
            Vector2(positions[x + 1] - positions[x], sprite->Bounds().Height()));

        auto realSpriteBounds = spriteBounds.AddPosition(bounds.TopLeft());

        //realSpriteBounds.SetCenter(RotateXY(bounds.GetCenter(), realSpriteBounds.GetCenter(), angle));

        //spriteBounds.SetCenter(RotateXY(bounds.Size() / 2, spriteBounds.GetCenter(), angle));

        Rectangle textureBounds(
            Vector2(texCoords[x], textureTopLeft.y),
            Vector2(texCoords[x + 1] - texCoords[x], uvBounds.BottomRight().y - textureTopLeft.y));

        Rectangle tbounds(spriteBounds.TopLeft(), textureBounds.Size());

        Sprite sprite(texture, tbounds, textureBounds);
        RenderSpriteRepeated(&sprite, realSpriteBounds, depth, bounds.GetCenter(), angle);
    }
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
    _spriteBatcher.RenderSolidColor(
        color,
        Vector3(
            rect.topLeft.x + _renderOffset.x,
            rect.topLeft.y + _renderOffset.y,
            depth),
        rect.Size(),
        nullptr,
        angle);
}

void Renderer::RenderLine(const Vector2 start, const Vector2 end, const Color color, float depth)
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

void Renderer::RenderCircle(Vector2 center, float radius, Color color, float depth, float subdivide)
{
    float dAngle = 2 * 3.14159 / subdivide;
    Vector2 last = center + Vector2(radius, 0);

    Vector2 v[128];

    for (int i = 0; i < subdivide; ++i)
    {
        float angle = dAngle * i;
        v[i] = center + Vector2(cos(angle), sin(angle)) * radius;
    }

    _spriteBatcher.RenderSolidPolygon(v, subdivide, depth, color);
}

void Renderer::DoRendering()
{
    // Only do debug rendering on the client
    if (!_scene->isServer)
    {
        for (auto& rect : _debugRectangles)
        {
            RenderRectangle(rect.rect, rect.color, -1, 0);
        }

        if (!_scene->GetEngine()->IsPaused()) _debugRectangles.clear();

        for (auto& line : _debugLines)
        {
            RenderLine(line.start, line.end, line.color, -1);
        }

        if (!_scene->GetEngine()->IsPaused()) _debugLines.clear();
    }

    if (g_useLighting.Value())
    {
        _deferredLightingColorBuffer->Bind();
        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 1);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
        _spriteBatcher.Render();

        _deferredLightingColorBuffer->Unbind();
        glClearColor(0, 0, 0, 1);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        _lightManager.UpdateVisibleLights(_camera, _scene, _deferredLightingColorBuffer->ColorBuffer());

        // Copy over the depth buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _deferredLightingColorBuffer->Id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        glBlitFramebuffer(0, 0,
            _deferredLightingColorBuffer->Width(),
            _deferredLightingColorBuffer->Height(), 0, 0,
            _deferredLightingColorBuffer->Width(),
            _deferredLightingColorBuffer->Height(),
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        _deferredLightingColorBuffer->Unbind();
    }
    else
    {
        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 1);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        _spriteBatcher.Render();
    }

    _lineRenderer.Render(_camera->ViewMatrix());
}

void Renderer::RenderSpriteBatch()
{
    glEnable(GL_DEPTH_TEST);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    _spriteBatcher.Render();
    _lineRenderer.Render(_camera->ViewMatrix());
}

void Renderer::BeginRender(Scene* scene, Camera* camera, Vector2 renderOffset, float deltaTime, float absoluteTime)
{
    _camera = camera;
    _scene = scene;
    _deltaTime = deltaTime;
    _absoluteTime = absoluteTime;

    _camera->RebuildViewMatrix();
    _renderOffset = renderOffset;

    _spriteBatcher.BeginRender(camera);
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

void Renderer::RenderCustomTransparency(const ICustomTransparencyRenderer* renderer, float depth)
{
    _spriteBatcher.RenderCustomTransparency(renderer, depth);
}

void Renderer::InitializeSpriteBatcher()
{
    const char* vertexShader = R"(
#version 330 core

uniform mat4x4 view;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 textureCoord;
layout (location = 2) in vec4 color;

out vec2 TexCoord;
out vec4 Color;
out vec2 FragPosition;

void main()
{
    gl_Position = view * vec4(aPos, 1.0);
    TexCoord = textureCoord;
    Color = color;
    FragPosition = aPos.xy;
})";

    const char* fragmentShader = R"(
#version 330 core

uniform sampler2D spriteTexture;
uniform sampler2D shadowMap;

in vec2 TexCoord;
in vec4 Color;
in vec2 FragPosition;

out vec4 FragColor;

void main()
{
    vec4 texel = texture(spriteTexture, TexCoord);

    if(texel.a == 0) discard;

    if(Color.a == 1)
    {
        float a = step(0.2, texel.a);

        FragColor = vec4(Color.rgb, a);
    }
    else
    {
        FragColor = vec4(mix(texel.rgb, Color.rgb, min(Color.a, 1)), texel.a);
    }
})";

    _spriteShader.Compile(vertexShader, fragmentShader);
    _spriteBatcher.Initialize(&_spriteShader);
}

void Renderer::ResizeScreen(int w, int h)
{
    _deferredLightingColorBuffer = std::make_unique<FrameBuffer>(TextureFormat::RGBA, TextureDataType::Float, w, h);
    _deferredLightingColorBuffer->AttachDepthAndStencilBuffer();
}