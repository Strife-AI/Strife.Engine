#pragma once

#include <cstdlib>
#include <algorithm>

#include "math.h"

static constexpr int NearestPowerOf2(const int value, const int powerOf2)
{
    return (value + powerOf2 - 1) & ~(powerOf2 - 1);
}

template <typename T>
static constexpr int SizeAsInt()
{
    return static_cast<int>(sizeof(T));
}

enum class ComparisonFunction
{
    Less,
    Equal,
    Greater,
};

float Step(float x, float edge, ComparisonFunction comparison = ComparisonFunction::Less);

template <typename T>
static constexpr T Clamp(const T& value, const T& minValue, const T& maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }
    if (value > maxValue)
    {
        return maxValue;
    }
    return value;
}

using byte = unsigned char;

template<typename T>
constexpr T Abs(const T& value)
{
    return value >= 0
        ? value
        : -value;
}

template<typename T>
constexpr T Min(const T& a, const T& b)
{
    return a < b
        ? a
        : b;
}

template<typename T>
constexpr T Max(const T& a, const T& b)
{
    return a < b
        ? b
        : a;
}

template<typename T>
constexpr T Lerp(const T& min, const T& max, float t)
{
    return min + (max - min) * t;
}

static inline bool IsApproximately(float value, float desiredValue, float tolerance = 0.00001f)
{
    return Abs(value - desiredValue) <= tolerance;
}

float AdjustAngle(float a);

// Gradually changes a value towards a desired goal over time.
float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float deltaTime,
                 float maxSpeed = INFINITY);

float SmoothDampAngle(float current, float target, float& currentVelocity, float smoothTime, float deltaTime,
    float maxSpeed = INFINITY);