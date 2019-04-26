#ifndef GEOMETRY_COMMON_H
#define GEOMETRY_COMMON_H

#include <map>

#include "singleton.hpp"
#include "buffer_unit.hpp"
#include "vertex_format.h"

namespace wcore
{

class GeometryCommon: public Singleton<GeometryCommon>
{
public:
    friend GeometryCommon& Singleton<GeometryCommon>::Instance();
    friend void Singleton<GeometryCommon>::Kill();

    void draw(hash_t hname);

private:
    GeometryCommon (const GeometryCommon&){};
    GeometryCommon();
   ~GeometryCommon();

   void add_mesh_3P(hash_t hname, Mesh<Vertex3P>* pmesh);
   void add_mesh_2P2U(hash_t hname, Mesh<Vertex2P2U>* pmesh);
   void add_mesh_line(hash_t hname, Mesh<Vertex3P>* pmesh);

   struct MeshInfo;

private:
    BufferUnit<Vertex3P>   buffer_unit_3P_;
    BufferUnit<Vertex3P>   buffer_unit_line_;
    BufferUnit<Vertex2P2U> buffer_unit_2P2U_;

    std::map<hash_t, MeshInfo> meshes_;
};

struct GeometryCommon::MeshInfo
{
    enum BufferIndex: uint8_t
    {
        BUFFER_3P,
        BUFFER_2P2U,
        BUFFER_LINE
    };

    size_t offset;
    size_t n_elem;
    BufferIndex buffer;
};

#define CGEOM GeometryCommon::Instance()

} // namespace wcore

#endif
