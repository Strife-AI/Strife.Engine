#include "SpriteBatcher.hpp"


#include "gl3w.h"
#include "Texture.hpp"
#include "Scene/Scene.hpp"
#include "Tools/ConsoleVar.hpp"

ConsoleVar<bool> g_allWhite("all-white", false);

void SpriteBatcher::RenderSprite(const Sprite& sprite, const Vector3 position, const Vector2 scale, float angle, bool flipHorizontal, Color blendColor)
{
    auto& uvBounds = sprite.UVBounds();
    auto& spriteBounds = sprite.Bounds();

    Vector2 center = position.XY() + (sprite.Bounds().Size() * scale / 2);

    Vector3 width(spriteBounds.Size().x * scale.x, 0, 0);
    Vector3 height(0, spriteBounds.Size().y * scale.y, 0);

    auto topLeftTex = uvBounds.TopLeft();
    auto bottomLeftTex = uvBounds.BottomLeft();

    auto topRightTex = uvBounds.TopRight();
    auto bottomRightTex = uvBounds.BottomRight();

    if (flipHorizontal)
    {
        std::swap(topLeftTex, topRightTex);
        std::swap(bottomLeftTex, bottomRightTex);
    }

    auto c = (g_allWhite.Value() ? Color(255, 255, 255, 255) :  blendColor).ToVector4();

    RenderVertex vertices[4] =
    {
        RenderVertex(position, topLeftTex, c),                                 // Top left
        RenderVertex(position + width, topRightTex, c),                // Top right
        RenderVertex(position + width + height, bottomRightTex, c),    // Bottom right
        RenderVertex(position + height, bottomLeftTex, c),             // Bottom left
    };

    Polygon poly(vertices);
    poly.RotateXY(center, angle);

    RenderPolygon(poly, sprite.GetTexture());
}

void SpriteBatcher::RenderSolidColor(Color color, const Vector3 position, const Vector2 size, Texture* textureOverride, float angle)
{
    Vector2 tex(0, 0);
    Vector4 colorv(color.ToVector4());

    colorv.w = 2;

    Vector3 width(size.x, 0, 0);
    Vector3 height(0, size.y, 0);

    RenderVertex vertices[4] =
    {
        RenderVertex(position, tex, colorv),                                // Top left
        RenderVertex(position + width, tex, colorv),                // Top right
        RenderVertex(position + width + height, tex, colorv),       // Bottom right
        RenderVertex(position + height, tex, colorv),               // Bottom left
    };

    Polygon poly(vertices);

    Texture* texture;

    if(textureOverride == nullptr)
    {
        if(color.a == 255)
        {
            texture = _solidColor.get();
        }
        else
        {
            texture = _transparencyTexture.get();

            for (auto& vertex : vertices)
            {
                vertex.textureCoord = Vector2(color.a / 255.0, 0);
            }
        }
    }
    else
    {
        texture = textureOverride;
    }

    poly.RotateXY(position.XY() + size / 2, angle);

    RenderPolygon(poly, texture);
}

void SpriteBatcher::RenderSolidPolygon(Vector2* vertices, int vertexCount, float z, Color color)
{
    RenderVertex v[32];

    for(int i = 0; i < vertexCount; ++i)
    {
        auto c = color.ToVector4();
        c.w = 2;

        v[i] = RenderVertex(Vector3(vertices[i].x, vertices[i].y, z), c);
    }

    Polygon poly(gsl::span<RenderVertex>(v, vertexCount));

    Texture* texture;

    if (color.a == 255)
    {
        texture = _solidColor.get();
    }
    else
    {
        texture = _transparencyTexture.get();

        for (auto& vertex : poly.vertices)
        {
            vertex.textureCoord = Vector2(color.a / 255.0, 0);
        }
    }

    RenderPolygon(poly, texture);
}

void SpriteBatcher::RenderCustomTransparency(const ICustomTransparencyRenderer* renderer, float z)
{
    _transparentObjects.PushBackIfRoom(TransparentObject(renderer, z));
}

void SpriteBatcher::BeginRender(Camera* camera)
{
    _camera = camera;
    _ignoreTransparency = false;
    _vertexResetSize = 0;
    _vertices.Clear();
}

void SpriteBatcher::Initialize(Shader* shader)
{
    _shader = shader;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &spriteVerticesVbo);
    glBindBuffer(GL_ARRAY_BUFFER, spriteVerticesVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RenderVertex) * MaxVerticesPerBatch, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * MaxElementsPerBatch, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(VertexLocation);
    glVertexAttribPointer(VertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, position));

    glEnableVertexAttribArray(TextureCoordLocation);
    glVertexAttribPointer(TextureCoordLocation, 2, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, textureCoord));

    glEnableVertexAttribArray(ColorLocation);
    glVertexAttribPointer(ColorLocation, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void*)offsetof(RenderVertex, color));

    glBindVertexArray(0);
    glBindBuffer(GL_VERTEX_ARRAY, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    _viewMatrixLocation = glGetUniformLocation(_shader->ProgramId(), "view");
    _lightPositionLocation = glGetUniformLocation(_shader->ProgramId(), "lightPosition");
    _botLightLocation = glGetUniformLocation(_shader->ProgramId(), "botLightPosition");
    _useLightingLocation = glGetUniformLocation(_shader->ProgramId(), "useLighting");
    _lightDistanceLocation = glGetUniformLocation(_shader->ProgramId(), "lightDistance");
    _totalPointLightsLocation = glGetUniformLocation(_shader->ProgramId(), "totalPointLights");

    _solidColor = std::make_unique<Texture>(Color(0, 0, 0, 255), 1, 1);

    Color transparencyTextureTexels[256];
    for(int i = 0; i < 256; ++i)
    {
        transparencyTextureTexels[i] = Color(0, 0, 0, i);
    }

    _transparencyTexture = std::make_unique<Texture>(transparencyTextureTexels, 256, 1);
    _transparencyTexture->UseNearestNeighbor();
}

void SpriteBatcher::Render()
{
    glViewport(0, 0, _camera->ScreenSize().x, _camera->ScreenSize().y);
    glBindVertexArray(vao);
    _shader->Use();

    glUniformMatrix4fv(_viewMatrixLocation, 1, GL_FALSE, &_camera->ViewMatrix()[0][0]);

    glUniform1i(glGetUniformLocation(_shader->ProgramId(), "spriteTexture"), 0);

    glBindBuffer(GL_ARRAY_BUFFER, spriteVerticesVbo);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        _vertexResetSize * sizeof(RenderVertex),
        sizeof(RenderVertex) * (_vertices.Size() - _vertexResetSize), _vertices.begin() + _vertexResetSize);

    glActiveTexture(GL_TEXTURE0);

    for (auto texture : _texturesRenderedThisFrame)
    {
        auto& renderInfo = texture->renderInfo;

        glBindTexture(GL_TEXTURE_2D, texture->Id());

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(int) * renderInfo.elements.size(), renderInfo.elements.data());

        glDrawElements(GL_TRIANGLES, renderInfo.elements.size(), GL_UNSIGNED_INT, (void*)0);

        renderInfo.elements.clear();
    }

    ++_currentFrame;
    _texturesRenderedThisFrame.Clear();

    if (!_ignoreTransparency)
    {
        // Sort the transparent objects
        std::sort(_transparentObjects.begin(), _transparentObjects.end());
        Texture* lastTexture = nullptr;

        for (auto& object : _transparentObjects)
        {
            if (object.customRenderer != nullptr)
            {
                _vertexResetSize = _vertices.Size();
                _ignoreTransparency = true;
                object.customRenderer->Render(this, _camera, object.z);
                _ignoreTransparency = false;
                lastTexture = nullptr;
                _vertices.Resize(_vertexResetSize);
                _vertexResetSize = 0;
            }
            else
            {
                auto texture = object.texture;

                if (texture != lastTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texture->Id());
                }

                glDrawArrays(GL_TRIANGLES, object.firstVertex, 3);
            }
        }

        _vertices.Clear();
        _transparentObjects.Clear();
    }
}

void SpriteBatcher::RenderPolygon(Polygon& polygon, Texture* texture)
{
    if(!polygon.Bounds().IntersectsWith(_camera->Bounds()))
    {
        return;
    }

    if ((texture->flags & PartiallyTransparent) && !_ignoreTransparency)
    {
        for (int i = 1; i < polygon.vertices.size() - 1; ++i)
        {
            int a = 0, b = i, c = i + 1;

            int firstVertex = _vertices.Size();
            _vertices.PushBack(polygon.vertices[a]);
            _vertices.PushBack(polygon.vertices[b]);
            _vertices.PushBack(polygon.vertices[c]);

            _transparentObjects.PushBackIfRoom(TransparentObject(firstVertex, polygon.vertices[0].position.z, texture));
        }

        return;
    }


    auto& renderInfo = texture->renderInfo;
    if (renderInfo.lastVisibleFrame != _currentFrame)
    {
        renderInfo.lastVisibleFrame = _currentFrame;
        _texturesRenderedThisFrame.PushBackIfRoom(texture);
    }

    int first =_vertices.Size();

    for(auto& v : polygon.vertices)
    {
        _vertices.PushBack(v);
    }

    for (int i = 1; i < polygon.vertices.size() - 1; ++i)
    {
        int a = first, b = first + i, c = first + i + 1;

        renderInfo.elements.push_back(a);
        renderInfo.elements.push_back(b);
        renderInfo.elements.push_back(c);
    }
}

int SpriteBatcher::UnformArrayLocation(const char* arrayName, const char* property, int index) const
{
    char name[1024];
    sprintf(name, "%s[%d].%s", arrayName, index, property);

    return glGetUniformLocation(_shader->ProgramId(), name);
}
