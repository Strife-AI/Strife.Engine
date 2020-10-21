#pragma once
#include <random>

#include "Renderer/Lighting.hpp"
#include "Math/Random.hpp"

enum class InterpolatorMode
{
    Hold,
    Linear,
    Forever,
    Pulse,
    Random
};

template<typename T>
struct Interpolator
{
    Interpolator(T* value_)
        : value(value_),
        target(*value_),
        delta(0)
    {
        
    }

    void Hold(T at)
    {
        *value = at;
        mode = InterpolatorMode::Hold;
    }

    void HoldCurrent()
    {
        mode = InterpolatorMode::Hold;
    }

    void LerpTo(T newTarget, T speed)
    {
        target = newTarget;
        delta = speed;
        mode = InterpolatorMode::Linear;
    }

    void AddForever(T speed)
    {
        mode = InterpolatorMode::Forever;
        delta = speed;
    }

    void Set(T val)
    {
        *value = val;
        mode = InterpolatorMode::Hold;
        delta = 0;
    }

    void Update(float deltaTime);

    void Pulse(float frequency)
    {
        delta = 2 * 3.14159 / frequency;
        mode = InterpolatorMode::Pulse;
        time = 0;
    }

    void Random(float chanceOfChanging)
    {
        delta = chanceOfChanging;
        mode = InterpolatorMode::Random;
    }

    T* value;
    T target;
    T delta;

    float time;
    float min;
    float max;
    float velocity = 0;

    InterpolatorMode mode = InterpolatorMode::Hold;

    
};

template <typename T>
void Interpolator<T>::Update(float deltaTime)
{
    time += deltaTime;

    if (mode == InterpolatorMode::Hold)
    {
        return;
    }
    else if (mode == InterpolatorMode::Linear)
    {
        *value += delta * deltaTime;

        if (delta < 0)
        {
            if (*value < target) *value = target;
        }
        else
        {
            if (*value > target) *value = target;
        }
    }
    else if (mode == InterpolatorMode::Forever)
    {
        *value += delta * deltaTime;
    }
    else if (mode == InterpolatorMode::Pulse)
    {
        float r = max - min;
        *value = min + r * (0.5 + cos(delta * time));
    }
    else if (mode == InterpolatorMode::Random)
    {
        if (time > 0.1)
        {
            if (Rand(0, 1) < delta)
            {
                target = 0.1;
            }
            else
            {
                target = max;
            }

            time = 0;
        }

        *value = SmoothDamp(*value, target, velocity, 0.05, deltaTime);
    }
}

struct LightAnimator
{
    void StartEffect(const std::string_view effect)
    {
        ApplyEffect(effect);
    }

    virtual void Update(float deltaTime) { }

    virtual void Stop() { }

    virtual ~LightAnimator() = default;

private:
    virtual void ApplyEffect(const std::string_view effect) { }
};


struct BaseLightAnimator : LightAnimator
{
    BaseLightAnimator(BaseLight* light)
        : intensity(&light->intensity),
        maxDistance(&light->maxDistance)
    {
        intensity.min = light->minIntensity;
        intensity.max = light->maxIntensity;
    }


    void ApplyEffect(const std::string_view effect) override;

    void Stop() override
    {
        intensity.HoldCurrent();
        maxDistance.HoldCurrent();
    }


    void Update(float deltaTime) override
    {
        intensity.Update(deltaTime);
        maxDistance.Update(deltaTime);
    }

    virtual ~BaseLightAnimator() = default;

    Interpolator<float> intensity;
    Interpolator<float> maxDistance;
};

struct SpotLightAnimator : BaseLightAnimator
{
    SpotLightAnimator(SpotLight* spotlight)
        : BaseLightAnimator(spotlight),
        beamAngle(&spotlight->beamAngle),
        facingAngle(&spotlight->facingAngle)
    {

    }

    void ApplyEffect(const std::string_view effect) override;

    void Update(float deltaTime) override
    {
        facingAngle.Update(deltaTime);
        beamAngle.Update(deltaTime);
        BaseLightAnimator::Update(deltaTime);
    }

    void Stop() override;

    Interpolator<float> beamAngle;
    Interpolator<float> facingAngle;
};

struct LampAnimator : BaseLightAnimator
{
    LampAnimator(PointLight* pointLight_, SpotLight* spotLight_)
        : BaseLightAnimator(pointLight_),
        pointLight(pointLight_),
        spotLight(spotLight_)
    {
        
    }

    void Update(float deltaTime) override;

    PointLight* pointLight;
    SpotLight* spotLight;
};