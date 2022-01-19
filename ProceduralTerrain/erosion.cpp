#include <glm/gtc/random.hpp>
#include "erosion.hpp"
#include "cube_sphere.hpp"


void Deposit(
    Merlin::CubemapData& heightmap,
    glm::vec3 position,
    glm::vec2 direction,
    float amount)
{
    auto coordinates = CubemapData::PointCoordinates(position);
    int resolution = heightmap.GetResolution();

    int i0 = (int)(coordinates.u * resolution - 0.5);
    int j0 = (int)(coordinates.v * resolution - 0.5);
    int i1 = (int)(coordinates.u * resolution + 0.5);
    int j1 = (int)(coordinates.v * resolution + 0.5);

    i0 = glm::max(0, glm::min(i0, resolution - 1));
    j0 = glm::max(0, glm::min(j0, resolution - 1));
    i1 = glm::max(0, glm::min(i1, resolution - 1));
    j1 = glm::max(0, glm::min(j1, resolution - 1));

    glm::vec2 w = glm::abs(direction) / (glm::abs(direction.x) + glm::abs(direction.y));

    float w00, w01, w10, w11 = 0.0f;

    if (direction.x > 0.0f && direction.y > 0.0f)
    {
        w00 = amount * 0.5;
        w10 = amount * 0.5 * w.y;
        w11 = amount * 0.0;
        w01 = amount * 0.5 * w.x;
    }
    else if (direction.x > 0.0f && direction.y < 0.0f)
    {
        w00 = amount * 0.5 * w.y;
        w10 = amount * 0.0;
        w11 = amount * 0.5 * w.x;
        w01 = amount * 0.5;
    }
    else if (direction.x < 0.0f && direction.y > 0.0f)
    {
        w00 = amount * 0.5 * w.x;
        w10 = amount * 0.5;
        w11 = amount * 0.5 * w.y;
        w01 = amount * 0.0;
    }
    else
    {
        w00 = amount * 0.0;
        w10 = amount * 0.5 * w.x;
        w11 = amount * 0.0;
        w01 = amount * 0.5 * w.y;
    }

    heightmap.GetPixel(coordinates.face, i0, j0, 0) += w00;
    heightmap.GetPixel(coordinates.face, i0, j1, 0) += w01;
    heightmap.GetPixel(coordinates.face, i1, j0, 0) += w10;
    heightmap.GetPixel(coordinates.face, i1, j1, 0) += w11;
}

void UpdateParticle(
    ErosionParticle& particle,
    Merlin::CubemapData& heightmap,
    const ErosionParameters& parameters)
{
    // Evaluate local surface geometry
    auto original_coordinates = CubemapData::PointCoordinates(particle.position);
    glm::vec3 sphere_normal = glm::normalize(particle.position);
    glm::vec3 eu = SphereHeightmapUTangent(particle.position, heightmap);
    glm::vec3 ev = SphereHeightmapVTangent(particle.position, heightmap);
    glm::vec3 surface_normal = glm::normalize(glm::cross(eu, ev));
    glm::vec3 gravity_direction = (
        surface_normal - glm::dot(surface_normal, sphere_normal) * sphere_normal);

    // Calculate particle state variables
    glm::vec3 original_position = particle.position;
    float original_altitude = BilinearInterpolate(heightmap, original_coordinates, 0);
    float spacing = 1.0f / heightmap.GetResolution();
    float speed = glm::length(particle.velocity);
    float soil_fraction_eq = glm::dot(particle.velocity, gravity_direction) * parameters.concentration_factor;
    soil_fraction_eq = glm::clamp(soil_fraction_eq, 0.0f, 1.0f);

    // Calculate maximum timestep
    float timestep = 0.2f * parameters.friction_time;
    timestep = glm::min(timestep, 0.2f * parameters.erosion_time);
    timestep = glm::min(timestep, 0.2f * parameters.evaporation_time);
    timestep = glm::min(timestep, spacing / (speed + 1.0e-8f));

    // Calculate time derivatives
    glm::vec3 d_velocity = gravity_direction - particle.velocity / parameters.friction_time;
    float d_volume = -particle.volume / parameters.evaporation_time;
    float d_fraction = (soil_fraction_eq - particle.soil_fraction) / parameters.erosion_time;
    d_fraction = glm::max(d_fraction, -timestep * particle.soil_fraction);
    
    // Update particle
    particle.velocity += timestep * d_velocity;
    particle.position += timestep * particle.velocity;
    particle.volume += timestep * d_volume;
    particle.soil_fraction += timestep * d_fraction;

    // Project movement variables back to sphere tangent frame
    particle.position = glm::normalize(particle.position);
    particle.velocity -= glm::dot(particle.velocity, particle.position);

    //
    auto new_coordinates = CubemapData::PointCoordinates(particle.position);
    auto new_altitude = BilinearInterpolate(heightmap, new_coordinates, 0);
    auto travel_distance = glm::length(particle.position - original_position);
    auto slope = (new_altitude - original_altitude) / (timestep * speed);

    // Deposit/Remove soil from heightmap
    float d_soil_volume = (
        particle.soil_fraction * d_volume +
        particle.volume * d_fraction);
    float d_height = -timestep * d_soil_volume / (spacing * spacing);
    d_height = glm::min(d_height, 0.5f * slope * spacing);
    glm::vec2 grid_direction(
        glm::dot(eu, particle.velocity),
        glm::dot(ev, particle.velocity));
    Deposit(heightmap, particle.position, grid_direction, d_height);

    // Reset Particles
    bool needs_reset = (particle.volume < 1.0e-3 * parameters.particle_start_volume);
    if (needs_reset)
        InitializeParticle(particle, parameters);
}

void InitializeParticle(
    ErosionParticle& particle,
    const ErosionParameters& parameters)
{
    particle.volume = parameters.particle_start_volume;
    particle.soil_fraction = 0.0f;
    particle.position = glm::normalize(glm::linearRand(
        glm::vec3(-1.0f),
        glm::vec3(+1.0f)));
    particle.velocity = glm::vec3(0.0f);
}