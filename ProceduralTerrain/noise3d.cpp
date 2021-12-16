#include "noise3d.hpp"


float FractalNoise(
    glm::vec3 point,
    float base_frequency,
    int octaves,
    float persistence,
    float frequency_multiplier)
{
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = base_frequency;
    for (int i = 0; i < octaves; ++i)
    {
        result += amplitude * SmoothNoise(frequency * point);
        amplitude *= persistence;
        frequency *= frequency_multiplier;
    }
    return result;
}

float FractalRidgeNoise(
    glm::vec3 point,
    float base_frequency,
    int octaves,
    float persistence,
    float frequency_multiplier)
{
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = base_frequency;
    for (int i = 0; i < octaves; ++i)
    {
        result += amplitude * SmoothRidgeNoise(frequency * point);
        amplitude *= persistence;
        frequency *= frequency_multiplier;
    }
    return result;
}
