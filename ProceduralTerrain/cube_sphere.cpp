#include "cube_sphere.hpp"
#include "Merlin/Render/cubemap.hpp"


glm::vec3 FaceToCube(
    const CubeFace& face,
    const float& u,
    const float& v)
{
    float uc = 2.0f * u - 1.0f;
    float vc = 2.0f * v - 1.0f;
    switch (face)
    {
    case PositiveX:
        return glm::vec3(+1.0f, -vc, -uc);
    case NegativeX:
        return glm::vec3(-1.0f, -vc, +uc);
    case PositiveY:
        return glm::vec3(+uc, +1.0f, +vc);
    case NegativeY:
        return glm::vec3(+uc, -1.0f, -vc);
    case PositiveZ:
        return glm::vec3(+uc, -vc, +1.0f);
    case NegativeZ:
        return glm::vec3(-uc, -vc, -1.0f);
    default:
        return glm::vec3(0.0);
    }
}

glm::vec3 CubeToSphere(const glm::vec3& p)
{
    auto x2 = p.x * p.x;
    auto y2 = p.y * p.y;
    auto z2 = p.z * p.z;

    auto x = p.x * glm::sqrt(1.0f - (y2 + z2) / 2 + (y2 * z2) / 3);
    auto y = p.y * glm::sqrt(1.0f - (z2 + x2) / 2 + (z2 * x2) / 3);
    auto z = p.z * glm::sqrt(1.0f - (x2 + y2) / 2 + (x2 * y2) / 3);

    return glm::vec3(x, y, z);
}

glm::vec2 FaceUVToCubemapUV(
    const CubeFace& face,
    const float& u,
    const float& v)
{
    glm::vec2 uv;
    switch (face)
    {
    case CubeFace::PositiveX:
        uv = glm::vec2(
            (2.0f + u) / 4.0f,
            (1.0f + v) / 3.0f);
        break;
    case CubeFace::NegativeX:
        uv = glm::vec2(
            (0.0f + u) / 4.0f,
            (1.0f + v) / 3.0f);
        break;
    case CubeFace::PositiveY:
        uv = glm::vec2(
            (1.0f + u) / 4.0f,
            (2.0f + v) / 3.0f);
        break;
    case CubeFace::NegativeY:
        uv = glm::vec2(
            (1.0f + u) / 4.0f,
            (0.0f + v) / 3.0f);
        break;
    case CubeFace::PositiveZ:
        uv = glm::vec2(
            (3.0f + u) / 4.0f,
            (1.0f + v) / 3.0f);
        break;
    case CubeFace::NegativeZ:
        uv = glm::vec2(
            (1.0f + u) / 4.0f,
            (1.0f + v) / 3.0f);
        break;
    default:
        uv = glm::vec2();
        break;
    }
    return glm::vec2(
        glm::clamp(uv.x, 0.0f, 1.0f),
        glm::clamp(uv.y, 0.0f, 1.0f));
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
                vertex.position = CubeToSphere(FaceToCube(face, face_u, face_v));
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
