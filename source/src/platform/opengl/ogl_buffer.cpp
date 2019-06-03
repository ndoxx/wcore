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

void OGLVertexArray::set_layout(std::uint64_t vertex_format_hash) const
{
    switch(vertex_format_hash)
    {
        case ctti::type_id<Vertex3P3N2U4C>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), nullptr);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), (const GLvoid*)(3*sizeof(float)));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), (const GLvoid*)(6*sizeof(float)));
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U4C), (const GLvoid*)(8*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
        }
        break;

        case ctti::type_id<Vertex3P3N3T2U>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), nullptr);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), (const GLvoid*)(3*sizeof(float)));
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), (const GLvoid*)(6*sizeof(float)));
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N3T2U), (const GLvoid*)(9*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
        }
        break;

        case ctti::type_id<VertexAnim>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), nullptr);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(3*sizeof(float)));
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(6*sizeof(float)));
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(9*sizeof(float)));
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VertexAnim), (const GLvoid*)(11*sizeof(float)));
            glVertexAttribIPointer(5, 4, GL_UNSIGNED_BYTE,  sizeof(VertexAnim), (const GLvoid*)(15*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
            glEnableVertexAttribArray(4);
            glEnableVertexAttribArray(5);
        }
        break;

        case ctti::type_id<Vertex3P3N2U>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U), nullptr);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U), (const GLvoid*)(3*sizeof(float)));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N2U), (const GLvoid*)(6*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
        }
        break;

        case ctti::type_id<Vertex3P3N>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N), nullptr);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3N), (const GLvoid*)(3*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
        }
        break;

        case ctti::type_id<Vertex3P2U>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P2U), nullptr);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3P2U), (const GLvoid*)(3*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
        }
        break;

        case ctti::type_id<Vertex3P>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P), nullptr);

            glEnableVertexAttribArray(0);
        }
        break;

        case ctti::type_id<Vertex3P3C>().hash():
        {
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3C), nullptr);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3P3C), (const GLvoid*)(3*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
        }
        break;

        case ctti::type_id<Vertex2P2U>().hash():
        {
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2P2U), nullptr);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2P2U), (const GLvoid*)(2*sizeof(float)));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
        }
        break;

        default:
            DLOGF("OpenGL VAO: cannot set layout for unknown vertex format.", "batch");
    }
}


} // namespace wcore
