#include "Polygon.hpp"

#include "gsl/span"

void SplitPolygonAlongPlane(
    gsl::span<Vector2> polygon,
    const Line& plane,
    gsl::span<Vector2>& frontSide,
    gsl::span<Vector2>& backSide)
{
    int totalFront = 0;
    int totalBack = 0;

    auto dot = plane.normal.Dot(polygon[0]);
    bool in = dot >= -plane.d;

    for (int i = 0; i < polygon.size(); ++i)
    {
        int next = (i + 1 < polygon.size() ? i + 1 : 0);

        if (in)
        {
            frontSide[totalFront++] = polygon[i];
        }
        else
        {
            backSide[totalBack++] = polygon[i];
        }

        auto nextDot = plane.normal.Dot(polygon[next]);
        bool nextIn = nextDot >= -plane.d;
        auto dotDiff = nextDot - dot;

        if (in != nextIn && dotDiff != 0)
        {
            auto scale = (-plane.d - dot) / dotDiff;

            frontSide[totalFront] = Lerp(polygon[i], polygon[next], scale);
            backSide[totalBack] = frontSide[totalFront];

            ++totalFront;
            ++totalBack;
        }

        dot = nextDot;
        in = nextIn;
    }

    frontSide = frontSide.subspan(0, totalFront);
    backSide = backSide.subspan(0, totalBack);

}