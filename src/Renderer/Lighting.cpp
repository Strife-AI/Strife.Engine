#include <box2d/b2_polygon_shape.h>

#include "Lighting.hpp"
#include "Camera.hpp"
#include "Engine.hpp"
#include "Renderer.hpp"
#include "GL/gl3w.h"
#include "Scene/Scene.hpp"

static const char* g_linearFalloffLightVertexShader = R"(
#version 330 core

uniform mat4x4 view;

layout (location = 0) in vec2 position;
out vec2 fragPosition;
out vec2 colorBufferCoord;

void main()
{
    gl_Position = view * vec4(position, 0, 1);
    fragPosition = position;
    colorBufferCoord = gl_Position.xy / 2 + vec2(0.5, 0.5);
}
)";

static const char* g_linearFalloffLightFragmentShader = R"(
#version 330 core

uniform vec2 lightPosition;
uniform float maxDistance;
uniform float intensity;
uniform vec3 color;
uniform sampler2D colorBuffer;

in vec2 fragPosition;
in vec2 colorBufferCoord;

out vec4 FragColor;

void main()
{
    vec3 sceneColor = texture(colorBuffer, colorBufferCoord).rgb;
    vec3 lightColor = intensity * clamp(1 - length(fragPosition - lightPosition) / maxDistance, 0, 1) * color;

    FragColor = vec4(sceneColor * lightColor, 1);
}

)";

static const char* g_lineLinearFalloffLightFragmentShader = R"(
#version 330 core

uniform float maxDistance;
uniform float intensity;
uniform vec3 color;

uniform vec2 normal;
uniform float d;

uniform sampler2D colorBuffer;

in vec2 fragPosition;
in vec2 colorBufferCoord;

out vec4 FragColor;

void main()
{
    vec3 sceneColor = texture(colorBuffer, colorBufferCoord).rgb;

    float dist = abs(dot(fragPosition, normal) + d);
    vec3 lightColor = intensity * clamp(1 - dist / maxDistance, 0, 1) * color;

    FragColor = vec4(sceneColor * lightColor, 1);
}

)";

static const char* g_stencilShadowVertexShader = R"(
#version 330 core

uniform mat4x4 view;

layout (location = 0) in vec2 position;

void main()
{
    gl_Position = view * vec4(position, 0, 1);
}
)";

static const char* g_stencilShadowFragmentShader = R"(
#version 330 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(0, 0, 0, 1);
}
)";

static const char* g_ambientLightVertexShader = R"(
#version 330 core

uniform mat4x4 view;

layout (location = 0) in vec2 position;

out vec2 fragPosition;
out vec2 colorBufferCoord;

void main()
{
    gl_Position = view * vec4(position, 0, 1);
    fragPosition = position;
    colorBufferCoord = gl_Position.xy / 2 + vec2(0.5, 0.5);
}
)";

static const char* g_ambientLightFragmentShader = R"(
#version 330 core

uniform float intensity;
uniform vec3 color;
uniform sampler2D colorBuffer;

in vec2 fragPosition;
in vec2 colorBufferCoord;

out vec4 FragColor;

void main()
{
    vec3 sceneColor = texture(colorBuffer, colorBufferCoord).rgb;
    vec3 lightColor = intensity * color;

    FragColor = vec4(sceneColor * lightColor, 1);
}

)";

LightManager::LightManager()
{
    glGenBuffers(1, &_lightVertexVbo);
    glBindBuffer(GL_ARRAY_BUFFER, _lightVertexVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2) * MaxLightVertices, nullptr, GL_DYNAMIC_DRAW);

    // Linear falloff light shader
    {
        _linearFalloffLightShader.Compile(g_linearFalloffLightVertexShader, g_linearFalloffLightFragmentShader);

        _linearFalloffLightShader.Use();
        glGenVertexArrays(1, &_shadowBufferVao);
        glBindVertexArray(_shadowBufferVao);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, (void*)0);
    }

    // Line light shader
    {
        _lineLightShader.Compile(g_linearFalloffLightVertexShader, g_lineLinearFalloffLightFragmentShader);

        _lineLightShader.Use();
        glGenVertexArrays(1, &_lineLightVao);
        glBindVertexArray(_lineLightVao);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, (void*)0);
    }

    // Ambient light shader
    {
        _ambientLightShader.Compile(g_ambientLightVertexShader, g_ambientLightFragmentShader);
        _ambientLightShader.Use();

        glGenVertexArrays(1, &_ambientLightVao);
        glBindVertexArray(_ambientLightVao);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, (void*)0);
    }

    // Stencil volumetric shadow shader
    {
        _stencilShadowShader.Compile(g_stencilShadowVertexShader, g_stencilShadowFragmentShader);
        _stencilShadowShader.Use();

        glGenVertexArrays(1, &_stencilShadowVao);
        glBindVertexArray(_stencilShadowVao);

        glGenBuffers(1, &_stencilShadowVertexVbo);
        glBindBuffer(GL_ARRAY_BUFFER, _stencilShadowVertexVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vector2) * MaxShadowVertices, nullptr, GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, (void*)0);
    }

    glBindVertexArray(0);
}

LightManager::~LightManager()
{
    glDeleteBuffers(1, &_lightVertexVbo);
}

void LightManager::AddLight(SpotLight* light)
{
    _spotLights.PushBackIfRoom(light);
}

void LightManager::AddLight(PointLight* light)
{
    _pointLights.PushBackIfRoom(light);
}

void LightManager::AddLight(AmbientLight* light)
{
    _ambientLights.PushBackIfRoom(light);
}

void LightManager::AddLight(CapsuleLight* light)
{
    _capsuleLights.PushBackIfRoom(light);
}

void LightManager::RemoveLight(SpotLight* light)
{
    visibleSpotLights.RemoveSingle(light);
    _spotLights.RemoveSingle(light);
}

void LightManager::RemoveLight(PointLight* light)
{
    _pointLights.RemoveSingle(light);
    visiblePointLights.RemoveSingle(light);
}

void LightManager::RemoveLight(AmbientLight* light)
{
    _ambientLights.RemoveSingle(light);
}

void LightManager::RemoveLight(CapsuleLight* light)
{
    _capsuleLights.RemoveSingle(light);
}

void AddHalfCircle(gsl::span<Vector2> vertices, Vector2 center, float radius, float startAngle)
{
    float dAngle = 3.14159 / vertices.size();

    for (int i = 0; i < vertices.size(); ++i)
    {
        float angle = startAngle + i * dAngle;
        vertices[i] = center + Vector2(cos(angle), sin(angle)) * radius;
    }
}

Vector2 RotateXY(Vector2 center, Vector2 vertex, float angle);

void CreateCapsuleOutline(gsl::span<Vector2> vertices, Vector2 center, float radius, float halfLength, float angle)
{
    int halfCount = vertices.size() / 2;
    AddHalfCircle(
        vertices.subspan(0, halfCount),
        RotateXY(center, center + Vector2(halfLength, 0), angle),
        radius,
        3 * 3.14159 / 2 + angle);

    AddHalfCircle(
        vertices.subspan(halfCount, halfCount),
        RotateXY(center, center - Vector2(halfLength, 0), angle),
        radius,
        3.14159 / 2 + angle);
}


void LightManager::RenderPointLight(int id, PointLight* pointLight)
{
    glUniform2f(glGetUniformLocation(id, "lightPosition"), pointLight->position.x, pointLight->position.y);
    glUniform1f(glGetUniformLocation(id, "maxDistance"), pointLight->maxDistance);
    glUniform1f(glGetUniformLocation(id, "intensity"), pointLight->intensity);

    auto c = pointLight->color.ToVector4();
    glUniform3f(glGetUniformLocation(id, "color"), c.x, c.y, c.z);

    auto size = Vector2(pointLight->maxDistance, pointLight->maxDistance);
    auto topLeft = pointLight->position - size;
    Rectangle bounds(topLeft, size * 2);

    Vector2 loop[32];

    float dAngle = pointLight->beamAngle / (2 * 3.14159);

    for (int i = 0; i < 32; ++i)
    {
        float angle = i * 2 * 3.14159f / 31.0f * dAngle + pointLight->facingAngle - pointLight->beamAngle / 2;
        loop[i] = pointLight->position + Vector2(cos(angle), sin(angle)) * pointLight->maxDistance;
    }

    Vector2 vertices[32 * 2 + 1];

    vertices[0] = pointLight->position;

    for (int i = 0; i < 31; ++i)
    {
        vertices[i * 2 + 1] = loop[i];
        vertices[i * 2 + 2] = loop[i + 1];
    }


    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector2) * 65 - 2, vertices);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 65 - 2);
}

void LightManager::UpdateVisibleLights(Camera* camera, Scene* scene, Texture* texture)
{
    auto cameraBounds = camera->Bounds();

    visibleSpotLights.Clear();

    // Find visible spotlights
    for (auto spotLight : _spotLights)
    {
        if (spotLight->isEnabled && (spotLight->Bounds().IntersectsWith(cameraBounds)))
        {
            visibleSpotLights.PushBack(spotLight);

            if (visibleSpotLights.Size() == visibleSpotLights.Capacity())
            {
                break;
            }
        }
    }

    // Find visible point lights
    visiblePointLights.Clear();

    for (auto pointLight : _pointLights)
    {
        if (pointLight->isEnabled && pointLight->Bounds().IntersectsWith(cameraBounds))
        {
            visiblePointLights.PushBack(pointLight);

            if (visiblePointLights.Size() == visiblePointLights.Capacity())
            {
                break;
            }
        }
    }

    {
        glBindVertexArray(_shadowBufferVao);

        _linearFalloffLightShader.Use();

        int id = _linearFalloffLightShader.ProgramId();

        glUniform1i(glGetUniformLocation(id, "colorBuffer"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->Id());

        glUniformMatrix4fv(glGetUniformLocation(id, "view"), 1, GL_FALSE, &camera->ViewMatrix()[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, _lightVertexVbo);

        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_ONE, GL_ONE);
        glDisable(GL_DEPTH_TEST);

        // Render point lights
        for (auto pointLight : visiblePointLights)
        {
            RenderPointLight(id, pointLight);
        }

        // Render capsule light ends
        for (auto capsuleLight : _capsuleLights)
        {
            if (!capsuleLight->isEnabled || !capsuleLight->Bounds().IntersectsWith(cameraBounds))
            {
                continue;
            }

            PointLight capsuleCap;

            capsuleCap.position = RotateXY(
                capsuleLight->position,
                capsuleLight->position - Vector2(capsuleLight->halfLength, 0),
                capsuleLight->angle);

            
            capsuleCap.color = capsuleLight->color;
            capsuleCap.maxDistance = capsuleLight->maxDistance;
            capsuleCap.intensity = capsuleLight->intensity;
            capsuleCap.beamAngle = 3.14159;
            capsuleCap.facingAngle = 3 * 3.14159 / 2 + capsuleLight->angle - capsuleCap.beamAngle / 2;

            RenderPointLight(id, &capsuleCap);

            capsuleCap.position = RotateXY(
                capsuleLight->position,
                capsuleLight->position + Vector2(capsuleLight->halfLength, 0),
                capsuleLight->angle);

            capsuleCap.facingAngle = 3.14159 / 2 + capsuleLight->angle - capsuleCap.beamAngle / 2;

            RenderPointLight(id, &capsuleCap);
        }
    }

    // Render capsule lights
    {
        glBindVertexArray(_lineLightVao);
        _lineLightShader.Use();

        int id = _lineLightShader.ProgramId();

        glUniform1i(glGetUniformLocation(id, "colorBuffer"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->Id());

        glUniformMatrix4fv(glGetUniformLocation(id, "view"), 1, GL_FALSE, &camera->ViewMatrix()[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, _lightVertexVbo);

        for (auto capsuleLight : _capsuleLights)
        {
            if (!capsuleLight->isEnabled || !capsuleLight->Bounds().IntersectsWith(cameraBounds))
            {
                continue;
            }

            auto v1 = RotateXY(
                capsuleLight->position,
                capsuleLight->position - Vector2(capsuleLight->halfLength, 0),
                capsuleLight->angle);

            auto v2 = RotateXY(
                capsuleLight->position,
                capsuleLight->position + Vector2(capsuleLight->halfLength, 0),
                capsuleLight->angle);

            auto dir = v2 - v1;
            auto axis = Vector2(-dir.y, dir.x).Normalize();
            glUniform2f(glGetUniformLocation(id, "normal"), axis.x, axis.y);

            auto d = -axis.Dot(v1);
            glUniform1f(glGetUniformLocation(id, "d"), d);

            glUniform1f(glGetUniformLocation(id, "maxDistance"), capsuleLight->maxDistance);

            glUniform1f(glGetUniformLocation(id, "intensity"), capsuleLight->intensity);

            auto c = capsuleLight->color.ToVector4();
            glUniform3f(glGetUniformLocation(id, "color"), c.x, c.y, c.z);

            Vector2 halfSize(capsuleLight->halfLength, capsuleLight->maxDistance);
            Rectangle bounds(capsuleLight->position - halfSize, halfSize * 2);

            Vector2 v[4] =
            {
                RotateXY(capsuleLight->position, bounds.TopLeft(), capsuleLight->angle),
                RotateXY(capsuleLight->position, bounds.TopRight(), capsuleLight->angle),
                RotateXY(capsuleLight->position, bounds.BottomRight(), capsuleLight->angle),
                RotateXY(capsuleLight->position, bounds.BottomLeft(), capsuleLight->angle),
            };

            Vector2 vertices[6] =
            {
                v[0], v[1], v[2],
                v[0], v[2], v[3]
            };

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector2) * 6, vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    // Render ambient lights
    {
        glBindVertexArray(_ambientLightVao);

        _ambientLightShader.Use();

        int id = _ambientLightShader.ProgramId();

        glUniform1i(glGetUniformLocation(id, "colorBuffer"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->Id());

        glUniformMatrix4fv(glGetUniformLocation(id, "view"), 1, GL_FALSE, &camera->ViewMatrix()[0][0]);

        glBindBuffer(GL_ARRAY_BUFFER, _lightVertexVbo);

        for (auto ambientLight : _ambientLights)
        {
            if (!ambientLight->bounds.IntersectsWith(cameraBounds))
            {
                continue;
            }

            glUniform1f(glGetUniformLocation(id, "intensity"), ambientLight->intensity);

            auto c = ambientLight->color.ToVector4();
            glUniform3f(glGetUniformLocation(id, "color"), c.x, c.y, c.z);

            Rectangle& bounds = ambientLight->bounds;

            Vector2 vertices[6] =
            {
                bounds.TopLeft(), bounds.TopRight(), bounds.BottomRight(),
                bounds.TopLeft(), bounds.BottomRight(), bounds.BottomLeft()
            };

            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector2) * 6, vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }

    // Render spot lights
    for (auto spotLight : visibleSpotLights)
    {
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);
        glEnable(GL_STENCIL_TEST);

        if (!RenderShadows(scene, spotLight, camera))
        {
            continue;
        }

        glBindVertexArray(_lightVertexVbo);
        _linearFalloffLightShader.Use();
        glBindBuffer(GL_ARRAY_BUFFER, _lightVertexVbo);

        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilMask(0);

        int id = _linearFalloffLightShader.ProgramId();

        glUniform2f(glGetUniformLocation(id, "lightPosition"), spotLight->position.x, spotLight->position.y);
        glUniform1f(glGetUniformLocation(id, "maxDistance"), spotLight->maxDistance);
        glUniform1f(glGetUniformLocation(id, "intensity"), spotLight->intensity);

        auto c = spotLight->color.ToVector4();
        glUniform3f(glGetUniformLocation(id, "color"), c.x, c.y, c.z);

        Vector2 loop[32];

        float dAngle = spotLight->beamAngle / (2 * 3.14159);

        for (int i = 0; i < 32; ++i)
        {
            float angle = i * 2 * 3.14159f / 31.0f * dAngle + spotLight->facingAngle - spotLight->beamAngle / 2;
            loop[i] = spotLight->position + Vector2(cos(angle), sin(angle)) * spotLight->maxDistance;
        }

        Vector2 vertices[32 * 2 + 1];

        vertices[0] = spotLight->position;

        for (int i = 0; i < 31; ++i)
        {
            vertices[i * 2 + 1] = loop[i];
            vertices[i * 2 + 2] = loop[i + 1];
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector2) * 65 - 2, vertices);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 65 - 2);
        glDisable(GL_STENCIL_TEST);
    }

    glDisable(GL_STENCIL_TEST);
    glDepthMask(GL_TRUE);
}

bool LightManager::RenderShadows(Scene* scene, SpotLight* spotLight, Camera* cam)
{
    static ColliderHandle overlappingColliders[8192];

    _shadowVertices.Clear();

    auto colliders = scene->FindOverlappingColliders(spotLight->Bounds(), overlappingColliders);


    for (auto collider : colliders)
    {
        if (!collider.OwningEntity()->flags.HasFlag(EntityFlags::CastsShadows))
        {
            continue;
        }

        auto fixture = collider.GetFixture();

        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                if (fixture->TestPoint(Scene::PixelToBox2D(spotLight->position + Vector2(x, y))))
                {
                    // Point is inside the polygon so the light is invisible
                   return false;
                }
            }
        }

        auto shape = fixture->GetShape();
        auto body = fixture->GetBody();

        switch (shape->GetType())
        {
        case b2Shape::e_circle: break;
        case b2Shape::e_edge: break;
        case b2Shape::e_polygon:
        {
            auto polygon = static_cast<b2PolygonShape*>(shape);

            bool edgeFacing[b2_maxPolygonVertices] = { false };
            Vector2 vertices[b2_maxPolygonVertices];

            auto lightPosition = spotLight->position;

            for (int i = 0; i < polygon->m_count; ++i)
            {
                auto vertex = Scene::Box2DToPixel(body->GetWorldPoint(polygon->m_vertices[i]));
                auto next = Scene::Box2DToPixel(body->GetWorldPoint(polygon->m_vertices[(i + 1) % polygon->m_count]));

                vertices[i] = vertex;

                auto edge = next - vertex;
                Vector2 normal(-edge.y, edge.x);

                edgeFacing[i] = normal.Dot(vertex - lightPosition) > 0;
            }


            for (int i = 0; i < polygon->m_count; ++i)
            {
                bool inShadow = edgeFacing[i];

                if (inShadow)
                {
                    auto scaled = (vertices[i] - lightPosition) * 5000;
                    auto nextScale = (vertices[(i + 1) % polygon->m_count] - lightPosition) * 5000;

                    Vector2 v[4] =
                    {
                        vertices[i],
                        vertices[(i + 1) % polygon->m_count],
                        nextScale,
                        scaled
                    };

                    // Could easily do an EBO if this requires too much data to be pushed over
                    // Or even better, put it all in a geometry shader
                    _shadowVertices.PushBackIfRoom(v[0]);
                    _shadowVertices.PushBackIfRoom(v[1]);
                    _shadowVertices.PushBackIfRoom(v[2]);

                    _shadowVertices.PushBackIfRoom(v[0]);
                    _shadowVertices.PushBackIfRoom(v[3]);
                    _shadowVertices.PushBackIfRoom(v[2]);
                }
            }

            break;
        }
        case b2Shape::e_chain: break;
        case b2Shape::e_typeCount: break;
        default:;
        }

    }

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);

    glBindVertexArray(_stencilShadowVao);
    _stencilShadowShader.Use();

    glUniformMatrix4fv(glGetUniformLocation(_stencilShadowShader.ProgramId(), "view"), 1, GL_FALSE, &cam->ViewMatrix()[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, _stencilShadowVertexVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vector2) * _shadowVertices.Size(), _shadowVertices.begin());

    glDrawArrays(GL_TRIANGLES, 0, _shadowVertices.Size());

    return true;
}