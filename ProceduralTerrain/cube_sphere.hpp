#ifndef CUBE_SPHERE_HPP
#define CUBE_SPHERE_HPP
#include <array>
#include "Merlin/Render/mesh.hpp"
#include "Merlin/Render/mesh_vertex.hpp"
#include "Merlin/Render/cubemap_data.hpp"


using namespace Merlin;


glm::vec3 CubeToSphere(const glm::vec3& p);

glm::vec3 SphereToCube(const glm::vec3& p);

glm::vec3 SphereHeightmapPoint(
    glm::vec3 direction,
    CubemapData& heightmap);

glm::vec3 SphereHeightmapPoint(
    CubemapCoordinates coordinates,
    CubemapData& heightmap);

glm::vec3 SphereHeightmapUTangent(
    glm::vec3 direction,
    CubemapData& heightmap);

glm::vec3 SphereHeightmapVTangent(
    glm::vec3 direction,
    CubemapData& heightmap);

std::shared_ptr<Mesh<Vertex_XNTBUV>> BuildSphereMesh(int n_face_divisions);

#endif