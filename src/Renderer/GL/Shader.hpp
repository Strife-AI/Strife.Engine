#pragma once

#include <string>
#include <memory>
#include <vector>

#include "glcorearb.h"
#include "gl3w.h"
#include "System/Logger.hpp"
#include "Math/Vector2.hpp"
#include "Math/Vector3.hpp"
#include "Math/Vector4.hpp"
#include "gsl/span"

class Shader
{
public:
    void Compile(const char* vertexSource, const char* fragmentSource, const char* geometrySource = nullptr);
    void Use();

    int ProgramId() const { return _id; }

private:
    void CheckCompileErrors(GLuint object, std::string type);

    int _id;
};

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

    void BufferSub(gsl::span<T> data)
    {
        glBindBuffer(glType, id);
        glBufferSubData(glType, 0, data.size_bytes(), data.data());
    }

    unsigned int id{};
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
inline OpenGlTypeMetadata GetOpenGlTypeMetadata();

template<>
inline OpenGlTypeMetadata GetOpenGlTypeMetadata<float>()
{
    return OpenGlTypeMetadata(GL_FLOAT, 1);
}

template<>
inline OpenGlTypeMetadata GetOpenGlTypeMetadata<int>()
{
    return OpenGlTypeMetadata(GL_INT, 1);
}

template<>
inline OpenGlTypeMetadata GetOpenGlTypeMetadata<Vector2>()
{
    return OpenGlTypeMetadata(GL_FLOAT, 2);
}

template<>
inline OpenGlTypeMetadata GetOpenGlTypeMetadata<Vector3>()
{
    return OpenGlTypeMetadata(GL_FLOAT, 3);
}

template<>
inline OpenGlTypeMetadata GetOpenGlTypeMetadata<Vector4>()
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

struct Texture;
struct Effect;
struct Camera;

struct RendererState
{
    RendererState();

    void BindTexture(ShaderUniform<Texture> uniform, Texture* texture, int textureUnit);
    void BindShader(int id);
    void BindVao(int id);
    void SetActiveEffect(Effect* effect);

    int activeTextureUnit = -1;
    int activeTextures[32];
    int activeShader = -1;
    int activeVao = -1;
    Effect* activeEffect = nullptr;
    Camera* camera;
};

struct Effect
{
    Effect(Shader* shader);

    void StartEffect();
    void StopEffect();
    void FlushEffect();

    int ProgramId() const { return shader->ProgramId(); }

    template<typename T>
    Vbo<T>* CreateBuffer(int count, VboType type);

    template<typename T, typename TSelector>
    void BindVertexAttribute(const char* name, Vbo<T>* vbo, const TSelector& selector, int divisor = 0);

    template<typename T>
    ShaderUniform<T> GetUniform(const char* name);

    std::vector<std::unique_ptr<IGraphicsObject>> graphicsObjects;
    unsigned int vao;
    Shader* shader;

    static RendererState* renderer;

    // TODO: rename to OnStart()
    virtual void Start() { }
    virtual void Stop() { }
    virtual void Flush() { }
};

template<typename T>
ShaderUniform<T> Effect::GetUniform(const char* name)
{
    int id = glGetUniformLocation(ProgramId(), name);
    return ShaderUniform<T>(id);
}

template<typename T>
Vbo<T>* Effect::CreateBuffer(int count, VboType type)
{
    auto vbo = new Vbo<T>(count, type);
    this->graphicsObjects.emplace_back(std::unique_ptr<IGraphicsObject>(vbo));
    return vbo;
}

template<typename T, typename TSelector>
void Effect::BindVertexAttribute(const char* name, Vbo<T>* vbo, const TSelector& selector, int divisor)
{
    using ElementPointerType = decltype(selector((T*)nullptr));
    static_assert(std::is_pointer_v<ElementPointerType>, "Return value to selector must be pointer");
    using ElementType = typename std::remove_pointer<ElementPointerType>::type;

    int stride = sizeof(T);
    size_t offset = (size_t)selector((T*)nullptr);

    auto attributeLocation = glGetAttribLocation(ProgramId(), name);

    glEnableVertexAttribArray(attributeLocation);
    auto typeMetadata = GetOpenGlTypeMetadata<ElementType>();
    glVertexAttribPointer(attributeLocation, typeMetadata.count, typeMetadata.type, GL_FALSE, stride, (void*)offset);

    if (divisor != 0)
    {
        glVertexAttribDivisor(attributeLocation, divisor);
    }
}