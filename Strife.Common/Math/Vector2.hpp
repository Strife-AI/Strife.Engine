#pragma once

#include <cmath>

#include "BitMath.hpp"

template <typename T>
struct Vector2Template
{
    constexpr Vector2Template()
        : x(0),
        y(0)
    {
    }

    constexpr Vector2Template(T x_, T y_)
        : x(x_),
        y(y_)
    {

    }

    constexpr Vector2Template(T value)
        : x(value),
        y(value)
    {
        
    }

    constexpr Vector2Template(const Vector2Template<T>& rhs) = default;

    float LengthSquared() const
    {
        return static_cast<float>(x) * x + static_cast<float>(y) * y;
    }

    float Length() const
    {
        return sqrtf(LengthSquared());
    }

    Vector2Template Normalize() const
    {
        float length = Length();

        if (length == 0)
        {
            return Zero();
        }
        return Vector2Template(
            x / length,
            y / length);
    }

    template <typename U>
    Vector2Template<U> AsVectorOfType() const
    {
        return Vector2Template<U>(
            static_cast<U>(x),
            static_cast<U>(y));
    }

    T Dot(const Vector2Template<T>& rhs) const
    {
        return x * rhs.x + y * rhs.y;
    }

    bool IsApproximately(const Vector2Template& v) const
    {
        return ::IsApproximately(x, v.x)
            && ::IsApproximately(y, v.y);
    }

    Vector2Template<int> Floor() const
    {
        return Vector2Template<int>(static_cast<T>(floor(x)), static_cast<int>(floor(y)));
    }

    Vector2Template Round() const
    {
        return Vector2(round(x), round(y));
    }

    Vector2Template Min(const Vector2Template& rhs) const
    {
        return Vector2Template(::Min(x, rhs.x), ::Min(y, rhs.y));
    }

    Vector2Template Max(const Vector2Template& rhs) const
    {
        return Vector2Template(::Max(x, rhs.x), ::Max(y, rhs.y));
    }

    Vector2Template Clamp(const Vector2Template& min, const Vector2Template& max)
    {
        return Vector2Template(
            ::Clamp(x, min.x, max.x),
            ::Clamp(y, min.y, max.y));
    }

    T& operator[](int index)
    {
        return (&x)[index];
    }

    const T& operator[](int index) const
    {
        return (&x)[index];
    }

    Vector2Template MaxLength(float maxLength) const
    {
        auto length = Length();

        if(length > maxLength)
        {
            return (*this) * (maxLength / length);
        }
        else
        {
            return *this;
        }
    }

    T TaxiCabDistance() const
    {
        return Abs(x) + Abs(y);
    }

    Vector2Template SmoothDamp(Vector2Template target, Vector2Template& velocity, float smoothSpeed, float deltaTime,
                               float maxSpeed = INFINITY);

    Vector2Template XVector() const
    {
        return Vector2Template(x, 0);
    }

    Vector2Template YVector() const
    {
        return Vector2Template(0, y);
    }

    static Vector2Template Zero()
    {
        return Vector2Template(0, 0);
    }

    static Vector2Template Unit()
    {
        return Vector2Template(1, 1);
    }

    T x;
    T y;
};

template <typename T>
Vector2Template<T> Vector2Template<T>::SmoothDamp(Vector2Template target, Vector2Template& velocity, float smoothSpeed, float deltaTime,
                                                  float maxSpeed)
{
    return Vector2Template(
            ::SmoothDamp(x, target.x, velocity.x, smoothSpeed, deltaTime, maxSpeed),
            ::SmoothDamp(y, target.y, velocity.y, smoothSpeed, deltaTime, maxSpeed));
}

template <typename T>
constexpr Vector2Template<T> operator+(const Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    return Vector2Template<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template <typename T>
constexpr Vector2Template<T>& operator+=(Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    lhs = Vector2Template<T>(lhs.x + rhs.x, lhs.y + rhs.y);

    return lhs;
}

template <typename T>
constexpr Vector2Template<T>& operator-=(Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    lhs = Vector2Template<T>(lhs.x - rhs.x, lhs.y - rhs.y);

    return lhs;
}

template <typename T>
constexpr Vector2Template<T> operator-(const Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    return Vector2Template<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template <typename T>
constexpr Vector2Template<T> operator*(const Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    return Vector2Template<T>(lhs.x * rhs.x, lhs.y * rhs.y);
}

template <typename T>
constexpr Vector2Template<T> operator/(const Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    return Vector2Template<T>(lhs.x / rhs.x, lhs.y / rhs.y);
}

template <typename T>
constexpr Vector2Template<T> operator*(const Vector2Template<T>& lhs, const float rhs)
{
    return Vector2Template<T>(lhs.x * rhs, lhs.y * rhs);
}

template <typename T>
constexpr Vector2Template<T> operator/(const Vector2Template<T>& lhs, const float rhs)
{
    return Vector2Template<T>(lhs.x / rhs, lhs.y / rhs);
}

template <typename T>
constexpr Vector2Template<T> operator-(const Vector2Template<T>& v)
{
    return Vector2Template<T>(-v.x, -v.y);
}

template <typename T>
constexpr bool operator==(const Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
constexpr bool operator!=(const Vector2Template<T>& lhs, const Vector2Template<T>& rhs)
{
    return !(lhs == rhs);
}


using Vector2 = Vector2Template<float>;
using Vector2i = Vector2Template<int>;
using Vector2s = Vector2Template<short>;