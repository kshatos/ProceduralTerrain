#include "cube_sphere.hpp"


glm::vec3 FaceToCube(
    const CubeFace& face,
    const float& u,
    const float& v)
{
    float uc = 2.0f * u - 1.0f;
    float vc = 2.0f * v - 1.0f;
    switch (face)
    {
    case CubeFace::PositiveX:
        return glm::vec3(1.0f, vc, uc);
    case CubeFace::NegativeX:
        return glm::vec3(-1.0f, vc, -uc);
    case CubeFace::PositiveY:
        return glm::vec3(uc, 1.0f, vc);
    case CubeFace::NegativeY:
        return glm::vec3(uc, -1.0f, -vc);
    case CubeFace::PositiveZ:
        return glm::vec3(-uc, vc, 1.0f);
    case CubeFace::NegativeZ:
        return glm::vec3(uc, vc, -1.0f);
    default:
        return glm::vec3();
    }
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

std::shared_ptr<Mesh<Vertex_XNUV>> BuildSphereMesh(int n_face_divisions)
{
    // Initialize mesh storage
    int n_vertices_per_face = n_face_divisions * n_face_divisions;
    int n_vertices = n_vertices_per_face * 6;

    int n_triangles_per_face = (n_face_divisions - 1) * (n_face_divisions - 1) * 2;
    int n_triangles = n_triangles_per_face * 6;

    auto mesh = std::make_shared<Mesh<Vertex_XNUV>>();
    mesh->SetVertexCount(n_vertices);
    mesh->SetTriangleCount(n_triangles);

    // Add data
    uint32_t vertex_index = 0;
    uint32_t triangle_index = 0;
    for (const auto& face : CubeFaces)
    {
        for (int i = 0; i < n_face_divisions; ++i)
        {
            float face_u = i / (n_face_divisions - 1.0f);
            for (int j = 0; j < n_face_divisions; ++j)
            {
                float face_v = j / (n_face_divisions - 1.0f);

                // Vertex
                auto& vertex = mesh->GetVertex(vertex_index);
                vertex.position = FaceToCube(face, face_u, face_v);
                vertex.normal = glm::normalize(vertex.position);
                vertex.uv = FaceUVToCubemapUV(face, face_u, face_v);

                // Triangles
                if (i != n_face_divisions - 1 && j != n_face_divisions - 1)
                {
                    mesh->GetIndex(triangle_index, 0) = vertex_index;
                    mesh->GetIndex(triangle_index, 1) = vertex_index + n_face_divisions + 1;
                    mesh->GetIndex(triangle_index, 2) = vertex_index + 1;
                    triangle_index++;

                    mesh->GetIndex(triangle_index, 0) = vertex_index;
                    mesh->GetIndex(triangle_index, 1) = vertex_index + n_face_divisions;
                    mesh->GetIndex(triangle_index, 2) = vertex_index + n_face_divisions + 1;
                    triangle_index++;
                }

                vertex_index++;
            }
        }
    }

    return mesh;
}
