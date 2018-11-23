#ifndef VERTEX_ARRAY_HPP
#define VERTEX_ARRAY_HPP

#include "buffer_unit.hpp"

namespace wcore
{

template <typename VertexT>
class VertexArray
{
private:
    GLuint VAO_;

public:
    explicit VertexArray(const BufferUnit<VertexT>& buffer_unit);
    ~VertexArray();

    void bind();
    void unbind();
};

template <typename VertexT>
VertexArray<VertexT>::VertexArray(const BufferUnit<VertexT>& buffer_unit):
VAO_(0)
{
    // Generate and init VAO
    glGenVertexArrays(1, &VAO_);
    glBindVertexArray(VAO_);
    buffer_unit.bind_buffers();
    buffer_unit.enable_vertex_attrib_array();
    glBindVertexArray(0);
}

template <typename VertexT>
VertexArray<VertexT>::~VertexArray(){}

template <typename VertexT>
void VertexArray<VertexT>::bind()
{
    glBindVertexArray(VAO_);
}

template <typename VertexT>
void VertexArray<VertexT>::unbind()
{
    glBindVertexArray(0);
}

}

#endif // VERTEX_ARRAY_HPP
