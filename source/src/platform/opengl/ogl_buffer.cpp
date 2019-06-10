#include <GL/glew.h>
#include <ctti/type_id.hpp>

#include "platform/opengl/ogl_buffer.h"
#include "vertex_format.h"
#include "logger.h"

namespace wcore
{

OGLVertexBuffer::OGLVertexBuffer(float* vertex_data, std::size_t size, bool dynamic):
rd_handle_(0)
{
    GLenum draw_type = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glGenBuffers(1, &rd_handle_);
    bind();
    glBufferData(GL_ARRAY_BUFFER, size, vertex_data, draw_type);

    DLOGI("OpenGL VBO created. id=" + std::to_string(rd_handle_), "batch");
}

OGLVertexBuffer::~OGLVertexBuffer()
{
    // Unbind and delete
    unbind();
    glDeleteBuffers(1, &rd_handle_);

    DLOGI("OpenGL VBO destroyed. id=" + std::to_string(rd_handle_), "batch");
}

void OGLVertexBuffer::bind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, rd_handle_);
}

void OGLVertexBuffer::unbind() const
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OGLVertexBuffer::stream(float* vertex_data, std::size_t size, std::size_t offset) const
{
    bind();
    glBufferSubData(GL_ARRAY_BUFFER, (GLuint)offset, size, vertex_data);
}



OGLIndexBuffer::OGLIndexBuffer(uint32_t* index_data, std::size_t size, bool dynamic):
rd_handle_(0)
{
    GLenum draw_type = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glGenBuffers(1, &rd_handle_);
    bind();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, index_data, draw_type);

    DLOGI("OpenGL IBO created. id=" + std::to_string(rd_handle_), "batch");
}

OGLIndexBuffer::~OGLIndexBuffer()
{
    // Unbind and delete
    unbind();
    glDeleteBuffers(1, &rd_handle_);

    DLOGI("OpenGL IBO destroyed. id=" + std::to_string(rd_handle_), "batch");
}

void OGLIndexBuffer::bind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd_handle_);
}

void OGLIndexBuffer::unbind() const
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OGLIndexBuffer::stream(uint32_t* index_data, std::size_t size, std::size_t offset) const
{
    bind();
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLuint)offset, size, index_data);
}




OGLVertexArray::OGLVertexArray()
{
    glGenVertexArrays(1, &rd_handle_);
    DLOGI("OpenGL VAO created. id=" + std::to_string(rd_handle_), "batch");
}

OGLVertexArray::~OGLVertexArray()
{
    glDeleteVertexArrays(1, &rd_handle_);
    DLOGI("OpenGL VAO destroyed. id=" + std::to_string(rd_handle_), "batch");
}

void OGLVertexArray::bind() const
{
    glBindVertexArray(rd_handle_);
}

void OGLVertexArray::unbind() const
{
    glBindVertexArray(0);
}

static GLenum shader_data_type_to_ogl_base_type(ShaderDataType type)
{
    switch(type)
    {
        case ShaderDataType::Float: return GL_FLOAT;
        case ShaderDataType::Vec2:  return GL_FLOAT;
        case ShaderDataType::Vec3:  return GL_FLOAT;
        case ShaderDataType::Vec4:  return GL_FLOAT;
        case ShaderDataType::Mat3:  return GL_FLOAT;
        case ShaderDataType::Mat4:  return GL_FLOAT;
        case ShaderDataType::Int:   return GL_INT;
        case ShaderDataType::IVec2: return GL_INT;
        case ShaderDataType::IVec3: return GL_INT;
        case ShaderDataType::IVec4: return GL_INT;
    }

    DLOGF("Unknown ShaderDataType", "batch");
    return 0;
};

void OGLVertexArray::set_layout(const BufferLayout& layout) const
{
    uint32_t index = 0;
    for(const auto& element: layout)
    {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index,
                              element.get_component_count(),
                              shader_data_type_to_ogl_base_type(element.type),
                              element.normalized ? GL_TRUE : GL_FALSE,
                              layout.get_stride(),
                              (const void*)(uint64_t)element.offset);
        ++index;
    }
}


} // namespace wcore
