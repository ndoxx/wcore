#ifndef BUFFER_H
#define BUFFER_H

#include <cstdint>
#include <utility>
#include <vector>

#include "wtypes.h"

namespace wcore
{

class VertexBuffer
{
public:
    VertexBuffer() {}
    virtual ~VertexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void stream(float* vertex_data, std::size_t size, std::size_t offset) const = 0;

    // Factory method to create the correct implementation
    // for the current renderer API
    static VertexBuffer* create(float* vertex_data, std::size_t size, bool dynamic=false);
};

class IndexBuffer
{
public:
    IndexBuffer() {}
    virtual ~IndexBuffer() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void stream(uint32_t* index_data, std::size_t size, std::size_t offset) const = 0;

    static IndexBuffer* create(uint32_t* index_data, std::size_t size, bool dynamic=false);
};

class BufferLayout;
class VertexArray
{
public:
    VertexArray() {}
    virtual ~VertexArray() {}

    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void set_layout(const BufferLayout& layout) const = 0;

    static VertexArray* create();
};

} // namespace wcore

#endif
