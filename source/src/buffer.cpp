#include "buffer.h"
#include "renderer_api.h"
#include "logger.h"

#include "platform/opengl/ogl_buffer.h"

namespace wcore
{

VertexBuffer* VertexBuffer::create(float* vertex_data, uint32_t size, bool dynamic)
{
    switch(RendererAPI::get_api())
    {
        case GfxAPI::None:
            DLOGF("VertexBuffer: not implemented for GfxAPI::None.", "buffer");
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLVertexBuffer(vertex_data, size, dynamic);
    }
}

IndexBuffer* IndexBuffer::create(uint32_t* index_data, uint32_t size, bool dynamic)
{
    switch(RendererAPI::get_api())
    {
        case GfxAPI::None:
            DLOGF("IndexBuffer: not implemented for GfxAPI::None.", "buffer");
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLIndexBuffer(index_data, size, dynamic);
    }
}


} // namespace wcore
