#include <GL/glew.h>

#include "platform/opengl/ogl_buffer.h"
#include "logger.h"

namespace wcore
{

OGLVertexBuffer::OGLVertexBuffer(float* vertex_data, std::size_t size, bool dynamic)
{
    GLenum draw_type = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glGenBuffers(1, &rd_handle_);
    bind();
    glBufferData(GL_ARRAY_BUFFER, size, vertex_data, draw_type);

    DLOGI("VBO created. id=" + std::to_string(rd_handle_), "batch");
}

OGLVertexBuffer::~OGLVertexBuffer()
{
    // Unbind and delete
    unbind();
    glDeleteBuffers(1, &rd_handle_);

    DLOGI("VBO destroyed. id=" + std::to_string(rd_handle_), "batch");
}

void OGLVertexBuffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
}

void OGLVertexBuffer::unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}




OGLIndexBuffer::OGLIndexBuffer(uint32_t* index_data, std::size_t size, bool dynamic)
{
    GLenum draw_type = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glGenBuffers(1, &rd_handle_);
    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, index_data, draw_type);

    DLOGI("IBO created. id=" + std::to_string(rd_handle_), "batch");
}

OGLIndexBuffer::~OGLIndexBuffer()
{
    // Unbind and delete
    unbind();
    glDeleteBuffers(1, &rd_handle_);

    DLOGI("IBO destroyed. id=" + std::to_string(rd_handle_), "batch");
}

void OGLIndexBuffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
}

void OGLIndexBuffer::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


} // namespace wcore
