#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "buffer_unit.hpp"
#include "vertex_array.hpp"
#include "mesh_factory.h"
#include "vertex_format.h"

template <typename VertexT>
class Renderer
{
protected:
    BufferUnit<VertexT>  buffer_unit_;
    VertexArray<VertexT> vertex_array_;

public:
    explicit Renderer():
    buffer_unit_(),
    vertex_array_(buffer_unit_){}

    ~Renderer() = default;

    void load_geometry() {}
    virtual void render() = 0;
};


struct Vertex3P;
template <typename U> class Mesh;
// template specialization for quad renderers
template <>
class Renderer<Vertex3P>
{
protected:
    BufferUnit<Vertex3P>  buffer_unit_;
    VertexArray<Vertex3P> vertex_array_;

public:
    explicit Renderer(GLenum primitive = GL_TRIANGLES):
    buffer_unit_(primitive),
    vertex_array_(buffer_unit_){}

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
    virtual void render() = 0;
};

#endif // RENDERER_HPP
