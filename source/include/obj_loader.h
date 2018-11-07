#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include "singleton.hpp"
#include "math3d.h"

template <typename VertexT> class Mesh;
struct Vertex3P3N3T2U;
using SurfaceMesh = Mesh<Vertex3P3N3T2U>;

#define TEXCOORD 0x01
struct Triangle
{
public:
    Triangle(): uvs(3){}

    math::i32vec3 indices;
    std::vector<math::vec3> uvs;
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

    SurfaceMesh* operator()(const char* objfile, bool process_uv=false);

};

#define LOADOBJ ObjLoader::Instance()

#endif // OBJ_LOADER_H
