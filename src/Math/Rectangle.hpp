#pragma once

#include <cstdlib>
#include <algorithm>

#include "Math/Vector2.hpp"
#include "Math/BitMath.hpp"
#include <gsl/span>

template <typename T>
struct RectangleTemplate
{
    using Vec2 = Vector2Template<T>;

    RectangleTemplate()
    {

    }

    RectangleTemplate(const T x, const T y, const T width, const T height)
        : topLeft(x, y),
          bottomRight(x + width, y + height)
    {
    }

    RectangleTemplate(const Vec2& position, const Vec2& size)
        : topLeft(position),
          bottomRight(position + size)
    {
    }

    T Width() const
    {
        return bottomRight.x - topLeft.x;
    }

    T Height() const
    {
        return bottomRight.y - topLeft.y;
    }

    T Area() const
    {
        return Width() * Height();
    }

    Vector2Template<T> Size() const
    {
        return bottomRight - topLeft;
    }

    void SetPosition(const Vec2& position)
    {
        auto size = Size();
        topLeft = position;
        bottomRight = position + size;
    }

    RectangleTemplate AddPosition(const Vec2& offset) const
    {
        return RectangleTemplate(topLeft + offset, Size());
    }

    void SetCenter(const Vec2& position)
    {
        SetPosition(position - Size() / 2);
    }

    Vec2 GetCenter() const
    {
        return (topLeft + bottomRight) / 2;
    }

    void SetSize(const Vec2& size)
    {
        bottomRight = topLeft + size;
    }

    RectangleTemplate Scale(Vector2 scale) const
    {
        return RectangleTemplate(TopLeft() * scale, Size() * scale);
    }

    Vector2 AlignInsideOf(const RectangleTemplate& outer) const;

    RectangleTemplate<T> Intersect(const RectangleTemplate& rhs) const;
    RectangleTemplate<T> Union(const RectangleTemplate& rhs) const;

    template<typename U>
    RectangleTemplate<U> As() const;

    Vec2 TopLeft() const { return topLeft; }
    Vec2 TopRight() const { return Vec2(bottomRight.x, topLeft.y); }
    Vec2 BottomLeft() const { return Vec2(topLeft.x, bottomRight.y); }
    Vec2 BottomRight() const { return bottomRight; }

    Vec2 TopCenter() const { return Vec2(GetCenter().x, topLeft.y); };
    Vec2 BottomCenter() const { return Vec2(GetCenter().x, bottomRight.y); };
    Vec2 LeftCenter() const { return Vec2(topLeft.x, GetCenter().y); };
    Vec2 RightCenter() const { return Vec2(bottomRight.x, GetCenter().y); };

    T Top() const { return topLeft.y; }
    T Bottom() const { return bottomRight.y; }
    T Left() const { return topLeft.x; }
    T Right() const { return bottomRight.x; }

    bool IntersectsWith(const RectangleTemplate& rhs) const;
    bool ContainsPoint(const Vec2& point) const;
    RectangleTemplate MinkowskiDifference(const RectangleTemplate& rhs) const;
    Vec2 ClosestPointOnBoundsToPoint(const Vec2& point);

    void GetPoints(gsl::span<Vector2Template<T>, 4> outPoints) const;

    static RectangleTemplate FromPoints(Vec2 a, Vec2 b);

    Vec2 topLeft;
    Vec2 bottomRight;
};

template <typename T>
Vector2 RectangleTemplate<T>::AlignInsideOf(const RectangleTemplate& outer) const
{
    Vector2 offset = Vector2(0, 0);

    if (Left() < outer.Left()) offset.x = outer.Left() - Left();
    else if (Right() > outer.Right()) offset.x = outer.Right() - Right();

    if (Top() < outer.Top()) offset.y = outer.Top() - Top();
    else if (Bottom() > outer.Bottom()) offset.y = outer.Bottom() - Bottom();

    return offset;
}

template <typename T>
RectangleTemplate<T> RectangleTemplate<T>::Intersect(const RectangleTemplate& rhs) const
{
    auto newTopLeft = Vector2Template<T>(
        std::max(topLeft.x, rhs.topLeft.x),
        std::max(topLeft.y, rhs.topLeft.y));

    auto newBottomRight = Vector2Template<T>(
        std::min(bottomRight.x, rhs.bottomRight.x),
        std::min(bottomRight.y, rhs.bottomRight.y));

    return RectangleTemplate(newTopLeft, newBottomRight - newTopLeft);
}

template <typename T>
RectangleTemplate<T> RectangleTemplate<T>::Union(const RectangleTemplate& rhs) const
{
    auto newTopLeft = topLeft.Min(rhs.topLeft);
    auto newBottomRight = bottomRight.Max(rhs.bottomRight);

    return RectangleTemplate(newTopLeft, newBottomRight - newTopLeft);
}

template <typename T>
template <typename U>
RectangleTemplate<U> RectangleTemplate<T>::As() const
{
    return RectangleTemplate<U>(
        topLeft.template AsVectorOfType<U>(),
        Size().template AsVectorOfType<U>());
}

template <typename T>
bool RectangleTemplate<T>::IntersectsWith(const RectangleTemplate& rhs) const
{
    bool doesNotIntersect = bottomRight.x < rhs.topLeft.x
        || rhs.bottomRight.x < topLeft.x
        || bottomRight.y < rhs.topLeft.y
        || rhs.bottomRight.y < topLeft.y;

    return !doesNotIntersect;
}

template <typename T>
bool RectangleTemplate<T>::ContainsPoint(const Vec2& point) const
{
    return point.x >= topLeft.x
        && point.x <= bottomRight.x
        && point.y >= topLeft.y
        && point.y <= bottomRight.y;
}

template <typename T>
RectangleTemplate<T> RectangleTemplate<T>::MinkowskiDifference(const RectangleTemplate<T>& rhs) const
{
    return RectangleTemplate(
        topLeft - rhs.bottomRight,
        Size() + rhs.Size());
}

template <typename T>
Vector2Template<T> RectangleTemplate<T>::ClosestPointOnBoundsToPoint(const Vec2& point)
{
    auto topLeftDist = Abs(point.x - topLeft.x);
    auto boundsPoint = Vec2(topLeft.x, point.y);

    if (Abs(bottomRight.x - point.x) < topLeftDist)
    {
        topLeftDist = Abs(bottomRight.x - point.x);
        boundsPoint = Vec2(bottomRight.x, point.y);
    }
    if (Abs(bottomRight.y - point.y) < topLeftDist)
    {
        topLeftDist = Abs(bottomRight.y - point.y);
        boundsPoint = Vec2(point.x, bottomRight.y);
    }
    if (Abs(topLeft.y - point.y) < topLeftDist)
    {
        topLeftDist = Abs(topLeft.y - point.y);
        boundsPoint = Vec2(point.x, topLeft.y);
    }

    return boundsPoint;
}

template <typename T>
void RectangleTemplate<T>::GetPoints(gsl::span<Vector2Template<T>, 4> outPoints) const
{
    outPoints[0] = TopLeft();
    outPoints[1] = TopRight();
    outPoints[2] = BottomRight();
    outPoints[3] = BottomLeft();
}

template <typename T>
RectangleTemplate<T> RectangleTemplate<T>::FromPoints(Vec2 a, Vec2 b)
{
    auto min = a.Min(b);
    auto max = a.Max(b);

    return RectangleTemplate(min, max - min);
}

using Rectangle = RectangleTemplate<float>;
using Rectanglei = RectangleTemplate<int>;
