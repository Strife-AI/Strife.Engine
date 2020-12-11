#pragma once

template<typename T>
struct Vector4Template
{
    Vector4Template()
        : x(0), y(0), z(0), w(0)
    {

    }

    Vector4Template(const T& x_, const T& y_, const T& z_, const T& w_)
        : x(x_),
        y(y_),
        z(z_),
        w(w_)
    {

    }

    T x;
    T y;
    T z;
    T w;
};

using Vector4 = Vector4Template<float>;