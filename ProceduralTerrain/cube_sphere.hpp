#ifndef CUBE_SPHERE_HPP
#define CUBE_SPHERE_HPP
#include <array>
#include "Merlin/Render/mesh.hpp"
#include "Merlin/Render/mesh_vertex.hpp"
using namespace Merlin;


enum class CubeFace
{
    PositiveX,
    NegativeX,
    PositiveY,
    NegativeY,
    PositiveZ,
    NegativeZ
};

static const std::array<CubeFace, 6> CubeFaces
{
    CubeFace::PositiveX,
    CubeFace::NegativeX,
    CubeFace::PositiveY,
    CubeFace::NegativeY,
    CubeFace::PositiveZ,
    CubeFace::NegativeZ
};


std::shared_ptr<Mesh<Vertex_XNUV>> BuildSphereMesh(int n_face_divisions);

#endif