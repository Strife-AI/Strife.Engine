#include <random>


#include "Math/Vector2.hpp"
#include "Random.hpp"


std::default_random_engine& GetLightGenerator()
{
    static std::default_random_engine generator;
    return generator;
}

float Rand(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(GetLightGenerator());
}

int Randi(int min, int max)
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(GetLightGenerator());
}

Vector2 Rand(Vector2 min, Vector2 max)
{
    return Vector2(
        Rand(min.x, max.x),
        Rand(min.y, max.y));
}

float RandomAngle(float startAngle, float endAngle)
{
    startAngle = AdjustAngle(startAngle);
    endAngle = AdjustAngle(endAngle);

    if (startAngle < endAngle)
    {
        return Rand(startAngle, endAngle);
    }
    else
    {
        return AdjustAngle(Rand(startAngle, endAngle + 2 * 3.14159));
    }
}
