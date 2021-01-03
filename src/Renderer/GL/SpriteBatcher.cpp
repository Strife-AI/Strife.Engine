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
    if(_transparentObjects.size() < MaxTransparentTriangles)
    {
        _transparentObjects.emplace_back(renderer, z);
    }
}

void SpriteBatcher::BeginRender(Camera* camera)
{
    _camera = camera;
    _ignoreTransparency = false;
    _vertexResetSize = 0;
    _vertices.clear();
}

struct IGraphicsObject
{
    virtual ~IGraphicsObject() = default;
};

enum class VboType
{
    Vertex,
    Element
};

template<typename T>
struct Vbo : IGraphicsObject
{
    Vbo(int count, VboType type, T* data = nullptr)
    {
        glGenBuffers(1, &id);

        switch (type)
        {
        case VboType::Vertex: glType = GL_ARRAY_BUFFER; break;
        case VboType::Element: glType = GL_ELEMENT_ARRAY_BUFFER; break;
        default: FatalError("Unknown vbo type: %d\n", (int)type);
        }

        glBindBuffer(glType, id);
        glBufferData(glType, sizeof(T) * count, data, GL_DYNAMIC_DRAW);
    }

    unsigned int id;
    unsigned int glType;
};

struct OpenGlTypeMetadata
{
    OpenGlTypeMetadata(GLenum type, int count)
        : type(type),
        count(count)
    {

    }

    GLenum type;
    int count;
};


template<typename T>
OpenGlTypeMetadata GetOpenGlTypeMetadata();

template<>
OpenGlTypeMetadata GetOpenGlTypeMetadata<float>()
{
    return OpenGlTypeMetadata(GL_FLOAT, 1);
}

template<>
OpenGlTypeMetadata GetOpenGlTypeMetadata<int>()
{
    return OpenGlTypeMetadata(GL_INT, 1);
}

template<>
OpenGlTypeMetadata GetOpenGlTypeMetadata<Vector2>()
{
    return OpenGlTypeMetadata(GL_FLOAT, 2);
}

template<>
OpenGlTypeMetadata GetOpenGlTypeMetadata<Vector3>()
{
    return OpenGlTypeMetadata(GL_FLOAT, 3);
}

template<>
OpenGlTypeMetadata GetOpenGlTypeMetadata<Vector4>()
{
    return OpenGlTypeMetadata(GL_FLOAT, 4);
}

template<typename T>
struct ShaderUniform
{
    ShaderUniform()
        : id(-1)
    {

    }

    ShaderUniform(int id)
        : id(id)
    {

    }

    int id;
};


struct BaseShader
{
    BaseShader(Shader* shader)
        : shader(shader)
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    }

    int ProgramId() const { return shader->ProgramId(); }

    template<typename T>
    Vbo<T>* CreateBuffer(int count, VboType type)
    {
        auto vbo = new Vbo<T>(count, type);
        graphicsObjects.emplace_back(std::unique_ptr<IGraphicsObject>(vbo));
        return vbo;
    }

    template<typename T, typename TSelector>
    void BindVertexAttribute(const char* name, Vbo<T>* vbo, const TSelector& selector)
    {
        using ElementPointerType = decltype(selector((T*)nullptr));
        static_assert(std::is_pointer_v<ElementPointerType>, "Return value to selector must be pointer");
        using ElementType = typename std::remove_pointer<ElementPointerType>::type;

        int stride = sizeof(T);
        size_t offset = (size_t)selector((T*)nullptr);

        auto attributeLocation = glGetAttribLocation(ProgramId(), name);
        Log("Location: %d\n", attributeLocation);

        glEnableVertexAttribArray(attributeLocation);
        auto typeMetadata = GetOpenGlTypeMetadata<ElementType>();
        glVertexAttribPointer(attributeLocation, typeMetadata.count, typeMetadata.type, GL_FALSE, stride, (void*)offset);
    }

    template<typename T>
    ShaderUniform<T> GetUniform(const char* name)
    {
        int id = glGetUniformLocation(ProgramId(), name);
        return ShaderUniform<T>(id);
    }

    std::vector<std::unique_ptr<IGraphicsObject>> graphicsObjects;
    unsigned int vao;
    Shader* shader;
};

struct SpriteShader : BaseShader
{
    SpriteShader(Shader* shader)
        : BaseShader(shader)
    {
        ebo = CreateBuffer<int>(SpriteBatcher::MaxElementsPerBatch, VboType::Element);
        vertices = CreateBuffer<RenderVertex>(SpriteBatcher::MaxVerticesPerBatch, VboType::Vertex);

        BindVertexAttribute("aPos", vertices, [=](auto rv) { return &rv->position; });
        BindVertexAttribute("textureCoord", vertices, [=](auto rv) { return &rv->textureCoord; });
        BindVertexAttribute("color", vertices, [=](auto rv) { return &rv->color; });

//        view = GetUniform<glm::mat4>("view");
    }

    Vbo<RenderVertex>* vertices;
    Vbo<int>* ebo;

    ShaderUniform<glm::mat4> view;
};

void SpriteBatcher::Initialize(Shader* shader)
{
    _shader = new SpriteShader(shader);

    _viewMatrixLocation = glGetUniformLocation(shader->ProgramId(), "view");

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * MaxElementsPerBatch, nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_VERTEX_ARRAY, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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
    glBindVertexArray(_shader->vao);
    _shader->shader->Use();

    glUniformMatrix4fv(_viewMatrixLocation, 1, GL_FALSE, &_camera->ViewMatrix()[0][0]);

    glUniform1i(glGetUniformLocation(_shader->ProgramId(), "spriteTexture"), 0);

    glBindBuffer(GL_ARRAY_BUFFER, _shader->vertices->id);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        _vertexResetSize * sizeof(RenderVertex),
        sizeof(RenderVertex) * (_vertices.size() - _vertexResetSize), _vertices.data() + _vertexResetSize);

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
                _vertexResetSize = _vertices.size();
                _ignoreTransparency = true;
                object.customRenderer->Render(this, _camera, object.z);
                _ignoreTransparency = false;
                lastTexture = nullptr;
                _vertices.resize(_vertexResetSize);
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

        _vertices.clear();
        _transparentObjects.clear();
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

            int firstVertex = _vertices.size();
            _vertices.push_back(polygon.vertices[a]);
            _vertices.push_back(polygon.vertices[b]);
            _vertices.push_back(polygon.vertices[c]);

            if(_transparentObjects.size() < MaxTransparentTriangles)
            {
                _transparentObjects.push_back(TransparentObject(firstVertex, polygon.vertices[0].position.z, texture));
            }
        }

        return;
    }


    auto& renderInfo = texture->renderInfo;
    if (renderInfo.lastVisibleFrame != _currentFrame)
    {
        renderInfo.lastVisibleFrame = _currentFrame;
        _texturesRenderedThisFrame.PushBackIfRoom(texture);
    }

    int first =_vertices.size();

    for(auto& v : polygon.vertices)
    {
        _vertices.push_back(v);
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
