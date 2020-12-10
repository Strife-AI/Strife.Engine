#pragma once

#include <Math/Vector4.hpp>

struct Color
{
    constexpr Color(short r_, short g_, short b_, short a_ = 255)
        : r(r_),
        g(g_),
        b(b_),
        a(a_)
    {

    }

    Color()
        : r(0), g(0), b(0), a(0)
    {

    }

    Color operator+(const Color& rhs) const
    {
        return Color(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
    }

    Color operator-(const Color& rhs) const
    {
        return Color(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a);
    }

    Color operator*(float scalar) const
    {
        return Color(r * scalar, g * scalar, b * scalar, a * scalar);
    }

    static constexpr Color Black()
    {
        return Color(0, 0, 0);
    }

	static constexpr Color Gray()
    {
        return Color(128, 128, 128);
    }

	static constexpr Color White()
    {
        return Color(255, 255, 255);
    }

    static constexpr Color Red()
    {
        return Color(255, 0, 0);
    }

	static constexpr Color Orange()
    {
        return Color(255, 165, 0);
    }

	static constexpr Color Yellow()
    {
        return Color(255, 255, 0);
    }

	static constexpr Color Green()
    {
        return Color(0, 255, 0);
    }

    static constexpr Color CornflowerBlue() { return Color(0, 162, 232); }

    static constexpr Color Blue()
    {
        return Color(0, 0, 255);
    }

    static constexpr Color Purple()
    {
	    return Color(128, 0, 128);
    }

	static constexpr Color Brown()
    {
        return Color(165, 42, 42);
    }

    static constexpr Color HotPink()
    {
	    return Color(252, 15, 192);
    }

    Vector4 ToVector4() const
    {
        return Vector4(
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            a / 255.0f);
    }

    unsigned int PackRGBA() const
    {
        return r | (g << 8) | (b << 16) | (a << 24);
    }

    static Color FromPacked(unsigned int rgba)
    {
        return Color(rgba & 255, rgba >> 8 & 255, rgba >> 16 & 255, rgba >> 24 & 255);
    }

    static Color FromVector4(const Vector4& v)
    {
        return Color(v.x * 255.0, v.y * 255.0, v.z * 255.0, v.w * 255.0);
    }

    short r, g, b, a;
};