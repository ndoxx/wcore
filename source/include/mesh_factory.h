#ifndef MESH_FACTORY_H
#define MESH_FACTORY_H

#include <map>
#include <exception>
#include <cstdint>

#include "mesh.hpp"

struct Vertex3P;
struct Vertex3P2U;
struct Vertex3P3N3T2U;
class FaceMesh;
class TriangularMesh;
class HeightMap;
class AABB;

namespace math
{
    namespace CSplineTangentPolicy
    {
        template <typename T> class CatmullRom;
    }
    template <typename T, typename Tg> class CSpline;
    template <unsigned N, typename T> class vec;
    using vec3 = vec<3, float>;
    using CSplineCatmullV3 = CSpline<vec3, CSplineTangentPolicy::CatmullRom<vec3>>;
}

namespace factory
{
    // 2D Meshes
    extern MeshP*  make_quad_3P();
    extern MeshPU* make_quad_3P2U();

    // 3D primitives
    extern FaceMesh* make_cube();
    extern FaceMesh* make_terrain(const HeightMap& hm,
                                  float latScale=1.0f,
                                  float texScale=1.0f);
    extern TriangularMesh* make_terrain_tri_mesh(const HeightMap& hm,
                                                 float latScale=1.0f,
                                                 float texScale=1.0f);
    extern FaceMesh*       make_uv_sphere(uint32_t nRings,
                                          uint32_t nRingPoints);
    extern TriangularMesh* make_ico_sphere(uint32_t refine=1,
                                           bool finalize=true);
    extern TriangularMesh* make_icosahedron(bool finalize=true);

    // 3D line primitives
    extern MeshP* make_cube_3P();
    extern MeshP* make_cube_NDC_3P();
    extern MeshP* make_uv_sphere_3P(uint32_t nRings,
                                    uint32_t nRingPoints,
                                    bool lines=false);
    extern MeshP* make_icosahedron_3P();

    // 3D procedural meshes
    extern FaceMesh* make_crystal(unsigned seed=0);
    extern FaceMesh* make_tentacle(const math::CSplineCatmullV3& spline,
                                   uint32_t nRings,
                                   uint32_t nRingPoints,
                                   float radius_0=0.1f,
                                   float radius_exponent=1.0f);

    // Helpers
    extern void skin_spline(FaceMesh* pmesh,
                            const math::CSplineCatmullV3& spline,
                            uint32_t nRings,
                            uint32_t nRingPoints,
                            float radius_0=0.1f,
                            float radius_exponent=1.0f);
    extern void subdivide_mesh(TriangularMesh* pmesh);
}

/*
template <typename MeshT, typename CreatorT, typename... Args>
class MeshFactory
{
private:
    std::map<uint32_t, CreatorT> creators_;

public:
    void register_creator(uint32_t index, CreatorT creator)
    {
        if(creators_.find(index) == creators_.end())
            creators_.insert(std::make_pair(index, creator));
        else
            throw std::runtime_error("Creator method already exists for index: " + std::to_string(index));
    }

    void make(uint32_t index, Args&&... args)
    {
        creators_.at(index)(std::forward<Args>(args)...);
    }
};
*/

#endif // MESH_FACTORY_H
