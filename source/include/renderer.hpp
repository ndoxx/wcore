#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "buffer_unit.hpp"
#include "mesh_factory.h"
#include "vertex_format.h"

namespace wcore
{

class Scene;
template <typename VertexT>
class Renderer
{
protected:
    BufferUnit<VertexT>  buffer_unit_;

public:
    explicit Renderer():
    buffer_unit_(){}

    ~Renderer() = default;

    void load_geometry() {}
    virtual void render(Scene* pscene) = 0;
};


struct Vertex3P;
template <typename U> class Mesh;
// template specialization for quad renderers
template <>
class Renderer<Vertex3P>
{
protected:
    BufferUnit<Vertex3P>  buffer_unit_;

public:
    explicit Renderer(GLenum primitive = GL_TRIANGLES):
    buffer_unit_(primitive){}

    ~Renderer() = default;

public:
    // by default load a quad
    void load_geometry()
    {
        Mesh<Vertex3P>* pquadMesh = factory::make_quad_3P();
        buffer_unit_.submit(*pquadMesh);
        buffer_unit_.upload();
        delete pquadMesh;
    }
    virtual void render(Scene* pscene) = 0;
};

}

#endif // RENDERER_HPP
