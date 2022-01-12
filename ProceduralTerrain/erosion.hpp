#ifndef EROSION_HPP
#define EROSION_HPP
#include <glm/glm.hpp>
#include "Merlin/Render/cubemap_data.hpp"


struct ErosionParameters
{
    float friction_time;
    float erosion_time;
    float evaporation_time;
    float particle_height;
    float particle_start_volume;
    float concentration_factor;
};


struct ErosionParticle
{
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float volume;
    float soil_fraction;

    ErosionParticle() :
        position(glm::vec3(0.0f)),
        direction(glm::vec3(0.0f)),
        speed(0.0f),
        volume(0.0f),
        soil_fraction(0.0f)
    {
    }
};


#endif