#include <random>

#include "LightAnimations.hpp"

void BaseLightAnimator::ApplyEffect(const std::string_view effect)
{
    if (effect == "fade-out")
    {
        intensity.LerpTo(intensity.min, -6);
    }
    else if (effect == "fade-in")
    {
        intensity.LerpTo(intensity.max, 6);
    }
    else if (effect == "off")
    {
        intensity.Set(intensity.min);
    }
    else if (effect == "on")
    {
        intensity.Set(intensity.max);
    }
    else if (effect == "pulse")
    {
        intensity.Pulse(1);
    }
    else if (effect == "flicker")
    {
        intensity.Random(0.5);
    }
}

void SpotLightAnimator::ApplyEffect(const std::string_view effect)
{
    if (effect == "spin-clockwise")
    {
        facingAngle.AddForever(1.5);
    }
    else if (effect == "spin-counter-clockwise")
    {
        facingAngle.AddForever(-1.5);
    }
    else
    {
        BaseLightAnimator::ApplyEffect(effect);
    }
}

void SpotLightAnimator::Stop()
{
    beamAngle.HoldCurrent();
    facingAngle.HoldCurrent();
    BaseLightAnimator::Stop();
}

void LampAnimator::Update(float deltaTime)
{
    BaseLightAnimator::Update(deltaTime);
    spotLight->intensity = (spotLight->maxIntensity / pointLight->maxIntensity) * pointLight->intensity;
}
