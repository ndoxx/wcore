#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <filesystem>

namespace fs = std::filesystem;

namespace wcore
{

template <typename VertexT> class Mesh;
struct Vertex3P3N3T2U;
using SurfaceMesh = Mesh<Vertex3P3N3T2U>;

class ObjLoader
{
public:
    std::shared_ptr<SurfaceMesh> load(std::istream& stream,
                                      bool process_uv=false,
                                      bool process_normals=false,
                                      int smooth_func=0);
    // deprec: use streams instead
    std::shared_ptr<SurfaceMesh> load(const char* objfile,
                                      bool process_uv=false,
                                      bool process_normals=false,
                                      int smooth_func=0);
    // deprec: use streams instead
    std::shared_ptr<SurfaceMesh> load(const fs::path& path,
                                      bool process_uv=false,
                                      bool process_normals=false,
                                      int smooth_func=0);
};

}

#endif // OBJ_LOADER_H
