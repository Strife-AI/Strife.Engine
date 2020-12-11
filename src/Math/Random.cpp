#include <random>


#include "Math/Vector2.hpp"

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