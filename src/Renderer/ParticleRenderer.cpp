#include "ParticleRenderer.hpp"

#include <random>
#include <GL/gl.h>



#include "GL/gl3w.h"
#include "Scene/Scene.hpp"
#include "Tools/ConsoleVar.hpp"

static std::default_random_engine& ParticleGenerator()
{
    static std::default_random_engine generator;
    return generator;
}

ConsoleVar<int> color("c", 10);
ConsoleVar<int> alpha("a", 40);
ConsoleVar<float> radius("r", 0.5);

void ParticleRenderer::SpawnParticle()
{
    float particleSize = 8;

    std::uniform_real_distribution<float> x(spawnRegion.Left() + particleSize / 2, spawnRegion.Right() - particleSize / 2);
    std::uniform_real_distribution<float> y(spawnRegion.Top() + particleSize / 2, spawnRegion.Bottom() - particleSize / 2);

    auto& gen = ParticleGenerator();
    Vector2 position(x(gen), y(gen));

    Particle p;
    p.position = position;

    std::uniform_int_distribution<int> pos(0, _particles.Size());
    std::uniform_real_distribution<float> xV(-10,  10);
    std::uniform_real_distribution<float> yV(-20,  -5);

    p.velocity = Vector2(xV(gen), -50);

    if (_particles.Size() == _particles.Capacity())
    {
        _particles.PopBack();
    }

    _particles.InsertBeforeIfRoom(p, 0);
}

void ParticleRenderer::SetSpawnRegion(Rectangle spawnRegion)
{
    this->spawnRegion = spawnRegion;
}

void ParticleRenderer::Render(SpriteBatcher* batcher, Camera* camera, float depth) const
{
    auto sprite = ResourceManager::GetResource<Sprite>("smoke"_sid);

    float scale = radius.Value();

    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    //glBlendEquation(GL_ADD);
    //glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_SUBTRACT);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    for (auto& particle : _particles)
    {

        //batcher->RenderSolidColor(
        //    Color(color.Value(), color.Value(), color.Value(), alpha.Value()),
        //    Vector3(particle.position, depth),
        //    Vector2(6, 6));

        batcher->RenderSprite(
            *sprite.Value(),
            Vector3(particle.position - sprite->Bounds().Size() * Vector2(scale, scale) / 2, depth),
            Vector2(scale, scale),
            0,
            false,
            Color(color.Value(), color.Value(), color.Value(), 254));
    }

    batcher->Render();

    glDepthMask(GL_TRUE);
    //glDisable(GL_BLEND);
}

void ParticleRenderer::Update(float deltaTime)
{
    deltaTime = 1.0 / 60;

    _particleSpawnTime += deltaTime;

    float spawnDelta = 1.0 / _spawnRate;
    while(_particleSpawnTime >= spawnDelta)
    {
        if(canSpawn)
            SpawnParticle();

        _particleSpawnTime -= spawnDelta;
    }

    for (auto& particle : _particles)
    {
        particle.position += particle.velocity * deltaTime;
    }
}
