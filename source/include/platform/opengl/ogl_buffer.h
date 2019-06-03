#ifndef OGL_BUFFER_H
#define OGL_BUFFER_H

#include "buffer.h"

namespace wcore
{

class OGLVertexBuffer: public VertexBuffer
{
public:
    OGLVertexBuffer(float* vertex_data, std::size_t size, bool dynamic=false);
    virtual ~OGLVertexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;

private:
    uint32_t rd_handle_;
};

class OGLIndexBuffer: public IndexBuffer
{
public:
    OGLIndexBuffer(uint32_t* index_data, std::size_t size, bool dynamic=false);
    virtual ~OGLIndexBuffer();

    virtual void bind() const override;
    virtual void unbind() const override;

private:
    uint32_t rd_handle_;
};

} // namespace wcore

#endif
