#ifndef CUBE_SPHERE_HPP
#define CUBE_SPHERE_HPP
#include <array>
#include "Merlin/Render/mesh.hpp"
#include "Merlin/Render/mesh_vertex.hpp"


using namespace Merlin;

std::shared_ptr<Mesh<Vertex_XNTBUV>> BuildSphereMesh(int n_face_divisions);

#endif