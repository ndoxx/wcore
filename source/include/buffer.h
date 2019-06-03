#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>

namespace wcore
{

class VertexBuffer
{
public:
    VertexBuffer() {}
    virtual ~VertexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    // Factory method to create the correct implementation
    // for the current renderer API
    static VertexBuffer* create(float* vertex_data, uint32_t size, bool dynamic=false);
};

class IndexBuffer
{
public:
    IndexBuffer() {}
    virtual ~IndexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    static IndexBuffer* create(uint32_t* index_data, uint32_t size, bool dynamic=false);
};

} // namespace wcore

#endif
