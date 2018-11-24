#ifndef BUFFER_UNIT_HPP
#define BUFFER_UNIT_HPP

#include <GL/glew.h>

#include "logger.h"
#include "mesh.hpp"

namespace wcore
{

static inline uint32_t dimensionality(GLenum primitive)
{
    switch(primitive)
    {
    case GL_QUADS:
        return 4;
    case GL_TRIANGLES:
        return 3;
    case GL_LINES:
        return 2;
    default:
        return 0;
    }
}

template <typename VertexT>
class BufferUnit
{
private:
    GLuint VBO_;
    GLuint IBO_;
    GLenum primitive_;

    std::vector<VertexT> vertices_;
    std::vector<GLuint>  indices_;

public:
    explicit BufferUnit(GLenum primitive = GL_TRIANGLES):
    VBO_(0),
    IBO_(0),
    primitive_(primitive)
    {
#ifdef __DEBUG__
        DLOGN("Generating Buffer Unit:", "buffer", Severity::LOW);
#endif
        glGenBuffers(1, &VBO_);
#ifdef __DEBUG__
        DLOGI("VBO created. id=" + std::to_string(VBO_), "buffer", Severity::LOW);
#endif
        glGenBuffers(1, &IBO_);
#ifdef __DEBUG__
        DLOGI("IBO created. id=" + std::to_string(IBO_), "buffer", Severity::LOW);
        DLOGI("dimensionality= " + std::to_string(dimensionality(primitive_)), "buffer", Severity::DET);
#endif
    }

    ~BufferUnit()
    {
        // BufferUnit objects are life-bound to their server-side counterparts
        // When they die, the OpenGL object dies as well
#ifdef __DEBUG__
        DLOGN("Destroying Buffer Unit:", "buffer", Severity::LOW);
#endif
        // First unbind
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Then delete buffers
        glDeleteBuffers(1,&IBO_);
#ifdef __DEBUG__
        DLOGI("IBO destroyed. id=" + std::to_string(IBO_), "buffer", Severity::LOW);
#endif
        glDeleteBuffers(1,&VBO_);
#ifdef __DEBUG__
        DLOGI("VBO destroyed. id=" + std::to_string(VBO_), "buffer", Severity::LOW);
#endif
        //check_gl_error();
    }

    inline GLuint get_vertex_buffer() const { return VBO_; }
    inline GLuint get_index_buffer() const  { return IBO_; }
    inline uint32_t get_n_vertices() const  { return vertices_.size(); }
    inline uint32_t get_n_indices() const   { return indices_.size(); }

    static inline void enable_vertex_attrib_array()
    {
        VertexT::enable_vertex_attrib_array();
    }

    void submit(Mesh<VertexT>& mesh)
    {
        uint32_t vert_offset = vertices_.size();
        mesh.set_buffer_offset(indices_.size());

        const std::vector<VertexT>& vertices = mesh.get_vertex_buffer();
        const std::vector<GLuint>&  indices  = mesh.get_index_buffer();

        // Add buffer offset to indices
        std::vector<GLuint> transformed_indices;
        transformed_indices.resize(indices.size());
        std::transform(indices.begin(), indices.end(), transformed_indices.begin(),
                       [=](GLuint ind) -> GLuint { return ind+vert_offset; });

        vertices_.insert(vertices_.end(),vertices.begin(),vertices.end());
        indices_.insert(indices_.end(),transformed_indices.begin(),transformed_indices.end());
    }

    void upload()
    {
        uint32_t nvert0 = vertices_.size();
        uint32_t nind0  = indices_.size();

        // Upload Vertex Data
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER,
                     nvert0*sizeof(VertexT),
                     &vertices_[0],
                     GL_STATIC_DRAW);

        // Upload Index Data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     nind0*sizeof(GLuint),
                     &indices_[0],
                     GL_STATIC_DRAW);
    }

    void upload_dynamic(uint32_t nvert, uint32_t nind)
    {
        // Vertex Data
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER,
                     nvert*sizeof(VertexT),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        // Index Data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     nind*sizeof(GLuint),
                     nullptr,
                     GL_DYNAMIC_DRAW);
    }

    void stream(const Mesh<VertexT>& mesh, uint32_t offset=0)
    {
        const std::vector<VertexT>& vertices = mesh.get_vertex_buffer();
        const std::vector<GLuint>&  indices  = mesh.get_index_buffer();

        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferSubData(GL_ARRAY_BUFFER, (GLuint)offset, mesh.get_nv()*sizeof(VertexT), &vertices[0]);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, (GLuint)offset, mesh.get_ni()*sizeof(GLuint), &indices[0]);
    }

    void bind_buffers() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
    }

    void draw(uint32_t n_elements, uint32_t offset, bool condition=true) const
    {
        if(n_elements==0 || !condition) return;

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
        glDrawElements(primitive_, dimensionality(primitive_)*n_elements, GL_UNSIGNED_INT,
                      (void*)(offset * sizeof(GLuint)));
    }
};

}

#endif // BUFFER_UNIT_HPP
