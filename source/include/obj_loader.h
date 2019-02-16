#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <filesystem>

#include "singleton.hpp"
#include "math3d.h"

namespace fs = std::filesystem;

namespace wcore
{

template <typename VertexT> class Mesh;
struct Vertex3P3N3T2U;
using SurfaceMesh = Mesh<Vertex3P3N3T2U>;

#define TEXCOORD 0x01
#define NORMAL   0x02
struct Triangle
{
public:
    math::i32vec3 indices;
    std::array<math::vec3,3> vertices;
    std::array<math::vec3,3> uvs;
    std::array<math::vec3,3> normals;
    int material;
    int attributes;
};

class ObjLoader: public Singleton<ObjLoader>
{
private:
    ObjLoader(const ObjLoader&) {}
    ObjLoader();
   ~ObjLoader();

public:
    friend ObjLoader& Singleton<ObjLoader>::Instance();
    friend void Singleton<ObjLoader>::Kill();

    std::shared_ptr<SurfaceMesh> operator()(const char* objfile,
                                            bool process_uv=false,
                                            bool process_normals=false,
                                            int smooth_func=0);
    std::shared_ptr<SurfaceMesh> operator()(const fs::path& path,
                                            bool process_uv=false,
                                            bool process_normals=false,
                                            int smooth_func=0);
};

#define LOADOBJ ObjLoader::Instance()

}

#endif // OBJ_LOADER_H
