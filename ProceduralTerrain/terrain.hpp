#ifndef TERRAIN_HPP
#define TERRAIN_HPP
#include <Merlin/Render/cubemap.hpp>
#include <Merlin/Render/cubemap_data.hpp>
#include <thread>
#include "noise3d.hpp"
#include "cube_sphere.hpp"


void GenerateNoiseHeightmap(std::shared_ptr<CubemapData>& height_data)
{
    std::vector<std::thread> threads;
    for (int face_id = Merlin::CubeFace::Begin; face_id < Merlin::CubeFace::End; face_id++)
    {
        auto work = [height_data, face_id]() {
            auto face = static_cast<Merlin::CubeFace>(face_id);
            for (int j = 0; j < height_data->GetResolution(); ++j)
            {
                for (int i = 0; i < height_data->GetResolution(); ++i)
                {
                    auto point = Merlin::CubemapData::CubePoint(height_data->GetPixelCoordinates(face, i, j));
                    point = glm::normalize(point);

                    float ridge_noise = 1.00f * FractalRidgeNoise(point, 2.0f, 4, 0.5f, 2.0f);
                    float smooth_noise = 0.05f * FractalNoise(point, 4.0f, 4, 0.7f, 2.0f);
                    float blend = 0.5f * (glm::simplex(1.0f * point) + 1.0f);
                    float noise = blend * ridge_noise + (1.0 - blend) * smooth_noise;

                    height_data->GetPixel(face, i, j, 0) = 0.5 + 0.1 * noise;
                }
            }
        };
        threads.emplace_back(work);
    }
    for (auto& thread : threads)
        thread.join();
}

void ErodeHeightmap(std::shared_ptr<CubemapData>& height_data)
{
    float grid_spacing = 1.0f / height_data->GetResolution();
    ErosionParameters erosion_params;
    erosion_params.concentration_factor = 3.0f;
    erosion_params.erosion_time = 0.5f;
    erosion_params.evaporation_time = 1.0f;
    erosion_params.friction_time = 0.5;
    erosion_params.particle_start_volume = 0.8f * grid_spacing * grid_spacing;

    int n_particles = 1000;
    int n_steps = 10000;
    std::vector<ErosionParticle> particles(n_particles);
    for (auto& p : particles) { InitializeParticle(p, erosion_params); }
    for (int i = 0; i < n_steps; ++i)
        for (auto& p : particles)
            UpdateParticle(p, *height_data, erosion_params);
}

void SmoothMap(std::shared_ptr<CubemapData>& map_data, int n_smooths)
{
    std::vector<std::thread> threads;
    threads.clear();
    for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
    {
        auto work = [map_data, face_id, n_smooths]() {
            auto face = static_cast<CubeFace>(face_id);
            for (int k = 0; k < n_smooths; ++k)
            {
                for (int j = 1; j < map_data->GetResolution() - 1; ++j)
                {
                    for (int i = 1; i < map_data->GetResolution() - 1; ++i)
                    {
                        float average = 0.25f * (
                            map_data->GetPixel(face, i + 1, j, 0) +
                            map_data->GetPixel(face, i - 1, j, 0) +
                            map_data->GetPixel(face, i, j + 1, 0) +
                            map_data->GetPixel(face, i, j - 1, 0));
                        map_data->GetPixel(face, i, j, 0) = average;
                    }
                }
            }
        };
        threads.emplace_back(work);
    }
    for (auto& thread : threads)
        thread.join();

}

void CalculateNormalMap(
    std::shared_ptr<CubemapData>& height_data,
    std::shared_ptr<CubemapData>& normal_data)
{
    std::vector<std::thread> threads;

    for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
    {
        auto work = [height_data, normal_data, face_id]() {
            auto face = static_cast<CubeFace>(face_id);
            for (int j = 0; j < height_data->GetResolution(); ++j)
            {
                for (int i = 0; i < height_data->GetResolution(); ++i)
                {
                    auto point = CubemapData::CubePoint(
                        height_data->GetPixelCoordinates(face, i, j));
                    point = glm::normalize(point);

                    auto eu = SphereHeightmapUTangent(point, *height_data);
                    auto ev = SphereHeightmapVTangent(point, *height_data);

                    auto normal = -glm::cross(eu, ev);
                    normal = glm::normalize(normal);
                    normal = 0.5f * (normal + 1.0f);
                    normal_data->GetPixel(face, i, j, 0) = normal.x;
                    normal_data->GetPixel(face, i, j, 1) = normal.y;
                    normal_data->GetPixel(face, i, j, 2) = normal.z;
                }
            }
        };
        threads.emplace_back(work);
    }
    for (auto& thread : threads)
        thread.join();
}

void GenerateBiomes(std::shared_ptr<CubemapData>& splat_data)
{
    std::vector<std::thread> threads;
    for (int face_id = CubeFace::Begin; face_id < CubeFace::End; face_id++)
    {
        auto work = [splat_data, face_id]() {
            auto face = static_cast<CubeFace>(face_id);
            for (int j = 0; j < splat_data->GetResolution(); ++j)
            {
                for (int i = 0; i < splat_data->GetResolution(); ++i)
                {
                    auto point = CubemapData::CubePoint(splat_data->GetPixelCoordinates(face, i, j));
                    point = glm::normalize(point);

                    float cosT = glm::dot(point, glm::vec3(0.0, 1.0, 0.0));
                    float T = glm::acos(cosT);
                    float sin2T = glm::sin(2.0f * T);

                    float temperature = 1.0f - cosT * cosT;
                    temperature += 0.1f * SmoothNoise(5.0f * point + glm::vec3(0.0, 15.0, 0.0));
                    temperature = glm::clamp(temperature, 0.0f, 1.0f);


                    float rainfall = sin2T * sin2T;
                    rainfall += 0.4f * SmoothNoise(3.0f * point + glm::vec3(0.0, 15.0, 0.0));
                    rainfall = glm::clamp(rainfall, 0.0f, 1.0f);

                    float tundra = glm::clamp(0.5f - (temperature - 0.3f) / 0.1f, 0.0f, 1.0f);

                    float shrub = (1.0f - tundra);
                    float grass = (1.0f - tundra);
                    float forest = (1.0f - tundra);
                    if (rainfall < 0.1)
                    {
                        grass *= 0.0f;
                        forest *= 0.0f;
                    }
                    else if (rainfall < 0.3)
                    {
                        float blend = 0.5f - (rainfall - 0.2f) / (2.0f * 0.1f);
                        shrub *= blend;
                        grass *= (1.0 - blend);
                        forest *= 0.0;
                    }
                    else if (rainfall < 0.5)
                    {
                        shrub *= 0.0f;
                        forest *= 0.0f;
                    }
                    else if (rainfall < 0.7f)
                    {
                        float blend = 0.5f - (rainfall - 0.6f) / (2.0f * 0.1f);
                        shrub *= 0.0f;
                        grass *= blend;
                        forest *= (1.0 - blend);
                    }
                    else
                    {
                        shrub *= 0.0f;
                        grass *= 0.0f;
                    }

                    splat_data->GetPixel(face, i, j, 0) = tundra;
                    splat_data->GetPixel(face, i, j, 1) = shrub;
                    splat_data->GetPixel(face, i, j, 2) = grass;
                    splat_data->GetPixel(face, i, j, 3) = forest;
                }
            }
        };
        threads.emplace_back(work);
    }
    for (auto& thread : threads)
        thread.join();
}

#endif