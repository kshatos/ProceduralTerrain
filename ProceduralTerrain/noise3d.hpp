#ifndef NOISE3D_HPP
#define NOISE3D_HPP
#include "Merlin/Render//cubemap_data.hpp"
#include "glm/gtc/noise.hpp"

using namespace Merlin;


inline float SmoothNoise(glm::vec3 point)
{
    return glm::simplex(point);
}

inline float SmoothRidgeNoise(glm::vec3 point)
{
    return 1.0f - 2.0f * glm::abs(glm::simplex(point));
}

float FractalNoise(
    glm::vec3 point,
    float base_frequency,
    int octaves,
    float persistence,
    float frequency_multiplier);

float FractalRidgeNoise(
    glm::vec3 point,
    float base_frequency,
    int octaves,
    float persistence,
    float frequency_multiplier);

#endif