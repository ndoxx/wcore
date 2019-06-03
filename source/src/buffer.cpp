#include "buffer.h"
#include "gfx_api.h"
#include "logger.h"

#include "platform/opengl/ogl_buffer.h"

namespace wcore
{

VertexBuffer* VertexBuffer::create(float* vertex_data, std::size_t size, bool dynamic)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGF("VertexBuffer: not implemented for GfxAPI::None.", "batch");
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLVertexBuffer(vertex_data, size, dynamic);
    }
}

IndexBuffer* IndexBuffer::create(uint32_t* index_data, std::size_t size, bool dynamic)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGF("IndexBuffer: not implemented for GfxAPI::None.", "batch");
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLIndexBuffer(index_data, size, dynamic);
    }
}

VertexArray* VertexArray::create()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGF("VertexArray: not implemented for GfxAPI::None.", "batch");
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLVertexArray();
    }
}

} // namespace wcore
