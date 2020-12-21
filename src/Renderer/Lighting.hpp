#pragma once
#include "Color.hpp"
#include "FrameBuffer.hpp"
#include "GL/Shader.hpp"
#include "Math/Rectangle.hpp"
#include "Math/Vector2.hpp"
#include "Memory/FixedLengthString.hpp"
#include "Memory/FixedSizeVector.hpp"
#include "Scene/Entity.hpp"

class Scene;
class Camera;
struct Entity;

struct BaseLight
{
    Vector2 position;
    Color color;
    float intensity;
    float maxDistance;
    bool isEnabled = true;
    float minIntensity = 0;
    float maxIntensity = 1;
};

struct SpotLight : BaseLight
{
    Rectangle Bounds() const
    {
        Vector2 size(maxDistance, maxDistance);

        return Rectangle(position - size, size * 2);
    }

    float facingAngle;
    float beamAngle;
    Entity* owner; // TODO: remove
};

struct PointLight : BaseLight
{
    Rectangle Bounds() const
    {
        Vector2 size(maxDistance, maxDistance);
        return Rectangle(position - size, size * 2);
    }

    Vector2 position;
    float facingAngle = 0;
    float beamAngle = 2 * 3.14159;
};

struct CapsuleLight : BaseLight
{
    Rectangle Bounds() const
    {
        float max = maxDistance * 2 + halfLength * 2;
        Vector2 approximateSize(max, max);
        return Rectangle(position - approximateSize / 2, approximateSize * 2);
    }

    float halfLength;
    float angle;
};

struct AmbientLight
{
    Rectangle bounds;
    Color color;
    float intensity;
};

class LightManager
{
public:
    static constexpr int MaxVisibleSpotLights = 128;
    static constexpr int MaxVisiblePointLights = 128;

    LightManager();
    ~LightManager();

    void AddLight(SpotLight* light);
    void AddLight(PointLight* light);
    void AddLight(AmbientLight* light);
    void AddLight(CapsuleLight* light);

    void RemoveLight(SpotLight* light);
    void RemoveLight(PointLight* light);
    void RemoveLight(AmbientLight* light);
    void RemoveLight(CapsuleLight* light);

    void RenderPointLight(int id, PointLight* pointLight);
    void UpdateVisibleLights(Camera* camera, Scene* scene, Texture* texture);

    FixedSizeVector<SpotLight*, 64> visibleSpotLights;
    FixedSizeVector<PointLight*, 64> visiblePointLights;

    FixedSizeVector<SpotLight*, 1024> _spotLights;
    FixedSizeVector<PointLight*, 1024> _pointLights;
    FixedSizeVector<AmbientLight*, 1024> _ambientLights;
    FixedSizeVector<CapsuleLight*, 1024> _capsuleLights;

private:
    static constexpr int MaxLightVertices = 128;
    static constexpr int MaxShadowVertices = 8192;

    bool RenderShadows(Scene* scene, SpotLight* spotLight, Camera* cam);

    Shader _linearFalloffLightShader;
    Shader _lineLightShader;
    Shader _ambientLightShader;

    unsigned int _lightVertexVbo;
    unsigned int _shadowBufferVao;
    unsigned int _ambientLightVao;
    unsigned int _lineLightVao;

    Shader _stencilShadowShader;
    unsigned int _stencilShadowVertexVbo;
    unsigned int _stencilShadowVao;
    FixedSizeVector<Vector2, MaxShadowVertices> _shadowVertices;
};