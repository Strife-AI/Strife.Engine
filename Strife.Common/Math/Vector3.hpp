#pragma once

#include "Vector2.hpp"

template<typename T>
struct Vector3Template
{
    constexpr Vector3Template()
        : x(0),
        y(0),
        z(0)
    {
        
    }

    constexpr Vector3Template(const T& x_,const T& y_, const T& z_)
        : x(x_),
        y(y_),
        z(z_)
    {

    }

    constexpr Vector3Template(Vector2Template<T> xy, float z_)
        : x(xy.x),
        y(xy.y),
        z(z_)
    {
        
    }

    [[nodiscard]]
    Vector2Template<T> XY() const
    {
        return Vector2Template<T>(x, y);
    }

    [[nodiscard]]
    Vector3Template<T> SetXY(const T& x_, const T& y_) const
    {
        return Vector3Template(x_, y_, z);
    }

    [[nodiscard]]
    Vector3Template<T> SetXY(const Vector2Template<T>& v) const
    {
        return Vector3Template(v.x, v.y, z);
    }

    T x;
    T y;
    T z;
};

using Vector3 = Vector3Template<float>;

template <typename T>
Vector3Template<T> operator+(const Vector3Template<T>& lhs, const Vector3Template<T>& rhs)
{
    return Vector3Template<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}