#ifndef RENDER_BATCH_HPP
#define RENDER_BATCH_HPP

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
class RenderBatch
{
private:
    GLuint VBO_;
    GLuint IBO_;
    GLuint VAO_;
    GLenum primitive_;
    hash_t category_;

    std::vector<VertexT> vertices_;
    std::vector<GLuint>  indices_;

public:
    explicit RenderBatch(hash_t category,
                         GLenum primitive = GL_TRIANGLES):
    VBO_(0),
    IBO_(0),
    VAO_(0),
    primitive_(primitive),
    category_(category)
    {
        DLOGN("New render batch:", "batch");
        DLOGI("Category: <n>" + HRESOLVE(category_) + "</n>", "batch");
        glGenBuffers(1, &VBO_);

        DLOGI("VBO created. id=" + std::to_string(VBO_), "batch");
        glGenBuffers(1, &IBO_);

        DLOGI("IBO created. id=" + std::to_string(IBO_), "batch");
        DLOGI("dimensionality= " + std::to_string(dimensionality(primitive_)), "batch");

        // Generate and init VAO
        glGenVertexArrays(1, &VAO_);
        glBindVertexArray(VAO_);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
        VertexT::enable_vertex_attrib_array();
        glBindVertexArray(0);

        DLOGI("VAO created. id=" + std::to_string(VAO_), "batch");
    }

    ~RenderBatch()
    {
        // RenderBatch objects are life-bound to their server-side counterparts
        // When they die, the OpenGL object dies as well
        DLOGN("Destroying render batch cat(<n>" + HRESOLVE(category_) + "</n>)", "batch");

        // First unbind
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Release VAO
        glDeleteVertexArrays(1, &VAO_);
        DLOGI("VAO destroyed. id=" + std::to_string(VAO_), "batch");

        // Then delete buffers
        glDeleteBuffers(1, &IBO_);
        DLOGI("IBO destroyed. id=" + std::to_string(IBO_), "batch");

        glDeleteBuffers(1, &VBO_);
        DLOGI("VBO destroyed. id=" + std::to_string(VBO_), "batch");
        //check_gl_error();
    }

    inline GLuint get_vertex_buffer() const { return VBO_; }
    inline GLuint get_index_buffer() const  { return IBO_; }
    inline uint32_t get_n_vertices() const  { return vertices_.size(); }
    inline uint32_t get_n_indices() const   { return indices_.size(); }

    void submit(Mesh<VertexT>& mesh)
    {
        uint32_t vert_offset = vertices_.size();
        mesh.set_buffer_offset(indices_.size());
        mesh.set_batch_category(category_);

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

    void upload(bool dynamic=false,
                uint32_t nvert=0,
                uint32_t nind=0)
    {
        nvert = (nvert==0) ? vertices_.size() : nvert;
        nind  = (nind==0) ? indices_.size() : nind;

        if(nvert==0 && nind==0)
            return;

        GLenum draw_type = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

#ifdef __DEBUG__
        size_t size_vertex_kb = (nvert * sizeof(VertexT)) / 1024;
        size_t size_index_kb  = (nind  * sizeof(GLuint))  / 1024;
        DLOGN("Sending render batch cat(<n>" + HRESOLVE(category_) + "</n>)", "batch");
        DLOGI("#vertices: " + std::to_string(nvert) + "/"
                            + std::to_string(vertices_.size()) + " -> <v>"
                            + std::to_string(size_vertex_kb) + "kB</v>", "batch");
        DLOGI("#indices:  " + std::to_string(nind) + "/"
                            + std::to_string(indices_.size()) + " -> <v>"
                            + std::to_string(size_index_kb) + "kB</v>", "batch");
#endif

        // Upload Vertex Data
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER,
                     nvert*sizeof(VertexT),
                     vertices_.data(),
                     draw_type);

        // Upload Index Data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     nind*sizeof(GLuint),
                     indices_.data(),
                     draw_type);
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

    void draw(uint32_t n_elements, uint32_t offset) const
    {
        if(n_elements==0) return;

        glBindVertexArray(VAO_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_);
        glDrawElements(primitive_, dimensionality(primitive_)*n_elements, GL_UNSIGNED_INT,
                      (void*)(offset * sizeof(GLuint)));
        //glBindVertexArray(0);
    }

    inline void draw(const BufferToken& buffer_token) const
    {
        draw(buffer_token.n_elements, buffer_token.buffer_offset);
    }
};

}

#endif // RENDER_BATCH_HPP
