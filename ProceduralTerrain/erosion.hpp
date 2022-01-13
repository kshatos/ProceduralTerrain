#ifndef EROSION_HPP
#define EROSION_HPP
#include <glm/glm.hpp>
#include "Merlin/Render/cubemap_data.hpp"


struct ErosionParameters
{
    float friction_time;
    float erosion_time;
    float evaporation_time;
    float particle_start_volume;
    float concentration_factor;
};


struct ErosionParticle
{
    glm::vec3 position;
    glm::vec3 velocity;
    float volume;
    float soil_fraction;

    ErosionParticle() :
        position(glm::vec3(0.0f)),
        velocity(glm::vec3(0.0f)),
        volume(0.0f),
        soil_fraction(0.0f)
    {
    }
};


void Deposit(
    Merlin::CubemapData& heightmap,
    glm::vec3 position,
    glm::vec2 direction,
    float amount);

void UpdateParticle(
    ErosionParticle& particle,
    Merlin::CubemapData& heightmap,
    const ErosionParameters& parameters);

void InitializeParticle(
    ErosionParticle& particle,
    const ErosionParameters& parameters);

#endif