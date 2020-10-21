#include "BitMath.hpp"

float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float deltaTime, float maxSpeed)
{
    if(deltaTime == 0)
    {
        return current;
    }

    // Based on Game Programming Gems 4 Chapter 1.10
    smoothTime = Max(0.0001F, smoothTime);
    float omega = 2.0f / smoothTime;

    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48F * x * x + 0.235F * x * x * x);
    float change = current - target;
    float originalTo = target;

    // Clamp maximum speed
    float maxChange = maxSpeed * smoothTime;
    change = Clamp(change, -maxChange, maxChange);
    target = current - change;

    float temp = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temp) * exp;
    float output = target + (change + temp) * exp;

    // Prevent overshooting
    if (originalTo - current > 0.0F == output > originalTo)
    {
        output = originalTo;
        currentVelocity = (output - originalTo) / deltaTime;
    }

    return output;
}

const float PI = 3.14159;

float AdjustAngle(float a)
{
    while (a < 0) a += 2 * PI;
    while (a >= 2 * PI) a -= 2 * PI;

    return a;
}

float DeltaAngle(float current, float target)
{
    float delta = AdjustAngle(target - current);
    if (delta > PI)
        delta -= 2 * PI;
    return delta;
}

float SmoothDampAngle(float current, float target, float& currentVelocity, float smoothTime, float deltaTime,
    float maxSpeed)
{
    target = current + DeltaAngle(current, target);
    return SmoothDamp(current, target, currentVelocity, smoothTime, deltaTime, maxSpeed);
}

float Step(float x, float edge, ComparisonFunction comparison)
{
    switch (comparison)
    {
        case ComparisonFunction::Less:
            return x < edge;
        case ComparisonFunction::Equal:
            return x == edge;
        case ComparisonFunction::Greater:
            return x > edge;
    }

    return 0.0f;
}
