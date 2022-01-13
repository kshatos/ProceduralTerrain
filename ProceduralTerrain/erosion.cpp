#include "erosion.hpp"
#include "cube_sphere.hpp"


void UpdateParticle(
    ErosionParticle& particle,
    Merlin::CubemapData& heightmap,
    const ErosionParameters& parameters)
{
    // Evaluate local surface geometry
    glm::vec3 sphere_normal = glm::normalize(particle.position);
    glm::vec3 eu = SphereHeightmapUTangent(particle.position, heightmap);
    glm::vec3 ev = SphereHeightmapVTangent(particle.position, heightmap);
    glm::vec3 surface_normal = glm::cross(eu, ev);
    glm::vec3 gravity_direction = (
        surface_normal - glm::dot(surface_normal, sphere_normal) * sphere_normal);

    // Calculate particle state variables
    glm::vec3 original_position = particle.position;
    float spacing = 1.0f / heightmap.GetResolution();
    float speed = glm::length(particle.velocity);
    float soil_fraction_eq = glm::dot(particle.velocity, gravity_direction) * parameters.concentration_factor;
    soil_fraction_eq = glm::clamp(soil_fraction_eq, 0.0f, 1.0f);

    // Calculate maximum timestep
    float timestep = 0.2f * parameters.friction_time;
    timestep = glm::min(timestep, 0.2f * parameters.erosion_time);
    timestep = glm::min(timestep, 0.2f * parameters.evaporation_time);
    timestep = glm::min(timestep, speed + 1.0e-8f);

    // Calculate time derivatives
    glm::vec3 d_velocity = gravity_direction - particle.velocity / parameters.friction_time;
    float d_volume = -particle.volume / parameters.evaporation_time;
    float d_concentration = (soil_fraction_eq - particle.soil_fraction) / parameters.erosion_time;

    // Update particle
    particle.velocity += timestep * d_velocity;
    particle.position += timestep * particle.velocity;

    // Project movement variables back to sphere tangent frame
    particle.position = glm::normalize(particle.position);
    particle.velocity -= glm::dot(particle.velocity, particle.position);

    // Deposit/Remove soil from heightmap
}
