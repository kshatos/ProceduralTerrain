#include "cube_sphere.hpp"


glm::vec3 CubeToSphere(const glm::vec3& p)
{
    /*
    auto x2 = p.x * p.x;
    auto y2 = p.y * p.y;
    auto z2 = p.z * p.z;

    auto x = p.x * glm::sqrt(1.0f - (y2 + z2) / 2 + (y2 * z2) / 3);
    auto y = p.y * glm::sqrt(1.0f - (z2 + x2) / 2 + (z2 * x2) / 3);
    auto z = p.z * glm::sqrt(1.0f - (x2 + y2) / 2 + (x2 * y2) / 3);
    
    return glm::vec3(x, y, z);
    */
    return glm::normalize(p);
}

glm::vec3 SphereToCube(const glm::vec3& p)
{
    float max = glm::abs(p.x);
    max = glm::max(glm::abs(p.y), max);
    max = glm::max(glm::abs(p.z), max);
    return p / max;
}

glm::vec3 SphereHeightmapPoint(
    glm::vec3 direction,
    CubemapData& heightmap)
{
    auto coordinates = CubemapData::PointCoordinates(direction);
    auto height = BilinearInterpolate(heightmap, coordinates, 0);
    return direction * (0.5f + height);
}

glm::vec3 SphereHeightmapPoint(
    CubemapCoordinates coordinates,
    CubemapData& heightmap)
{
    auto direction = CubeToSphere(CubemapData::CubePoint(coordinates));
    auto height = BilinearInterpolate(heightmap, coordinates, 0);
    return direction * (0.5f + height);
}

glm::vec3 SphereHeightmapUTangent(
    glm::vec3 direction,
    CubemapData& heightmap)
{
    float step = 1.0f / heightmap.GetResolution();

    auto coordinates = CubemapData::PointCoordinates(direction);
    auto p0 = SphereHeightmapPoint(coordinates, heightmap);

    coordinates.u += step;
    auto p1 = SphereHeightmapPoint(coordinates, heightmap);

    auto tangent = (p1 - p0) / step;

    return tangent;
}

glm::vec3 SphereHeightmapVTangent(
    glm::vec3 direction,
    CubemapData& heightmap)
{
    float step = 1.0f / heightmap.GetResolution();

    auto coordinates = CubemapData::PointCoordinates(direction);
    auto p0 = SphereHeightmapPoint(coordinates, heightmap);

    coordinates.v += step;
    auto p1 = SphereHeightmapPoint(coordinates, heightmap);

    auto tangent = (p1 - p0) / step;

    return tangent;
}

std::shared_ptr<Mesh<Vertex_XNTBUV>> BuildSphereMesh(int n_face_divisions)
{
    // Initialize mesh storage
    int n_vertices_per_face = n_face_divisions * n_face_divisions;
    int n_vertices = n_vertices_per_face * 6;

    int n_triangles_per_face = (n_face_divisions - 1) * (n_face_divisions - 1) * 2;
    int n_triangles = n_triangles_per_face * 6;

    auto mesh = std::make_shared<Mesh<Vertex_XNTBUV>>();
    mesh->SetVertexCount(n_vertices);
    mesh->SetTriangleCount(n_triangles);

    // Add data
    uint32_t vertex_index = 0;
    uint32_t triangle_index = 0;
    for (int face_id = CubeFace::Begin; face_id < CubeFace::End; ++face_id)
    {
        auto face = static_cast<CubeFace>(face_id);
        for (int i = 0; i < n_face_divisions; ++i)
        {
            float face_u = i / (n_face_divisions - 1.0f);
            for (int j = 0; j < n_face_divisions; ++j)
            {
                float face_v = j / (n_face_divisions - 1.0f);

                // Vertex
                auto& vertex = mesh->GetVertex(vertex_index);
                vertex.position = CubeToSphere(
                    CubemapData::CubePoint(
                        CubemapCoordinates{ face, face_u, face_v }));
                vertex.normal = glm::normalize(vertex.position);
                vertex.uv = glm::vec2(face_u, face_v);

                // Triangles
                if (i != n_face_divisions - 1 && j != n_face_divisions - 1)
                {
                    mesh->GetIndex(triangle_index, 0) = vertex_index;
                    mesh->GetIndex(triangle_index, 1) = vertex_index + 1;
                    mesh->GetIndex(triangle_index, 2) = vertex_index + n_face_divisions + 1;
                    triangle_index++;

                    mesh->GetIndex(triangle_index, 0) = vertex_index;
                    mesh->GetIndex(triangle_index, 1) = vertex_index + n_face_divisions + 1;
                    mesh->GetIndex(triangle_index, 2) = vertex_index + n_face_divisions;
                    triangle_index++;
                }

                vertex_index++;
            }
        }
    }

    return mesh;
}
