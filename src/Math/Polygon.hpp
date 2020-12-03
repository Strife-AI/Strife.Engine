#pragma once

#include <vector>
#include "Math/Vector2.hpp"
#include "gsl/span"

template<typename T>
struct PolygonTemplate
{
    PolygonTemplate()
    {

    }

    PolygonTemplate(const std::vector<Vector2Template<T>>& points_)
            : points(points_)
    {
        
    }

    void AddPoint(const Vector2Template<T> &point)
    {
        points.push_back(point);
    }

    bool IsEmpty() const
    {
        return points.empty();
    }

    std::vector<Vector2Template<T>> points;

private:
    bool _isEmpty = true;
};

using Polygonf = PolygonTemplate<float>;
using Polygoni = PolygonTemplate<int>;

struct Line
{
    Line(float angle, Vector2 point)
    {
        normal = { cos(angle), sin(angle) };
        d = -normal.Dot(point);
    }

    Vector2 normal;
    float d;
};

void SplitPolygonAlongPlane(
    gsl::span<Vector2> polygon,
    const Line& plane,
    gsl::span<Vector2>& frontSide,
    gsl::span<Vector2>& backSide);