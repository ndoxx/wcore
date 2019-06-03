#ifndef RENDER_BATCH_HPP
#define RENDER_BATCH_HPP

#include <GL/glew.h>
#include <ctti/type_id.hpp>

#include "logger.h"
#include "mesh.hpp"

#include "buffer.h"

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

// REFACTOR
// [ ] Make OpenGL agnostic
// [ ] Let meshes handle index buffers
//      -> 1 submesh = 1 index buffer & 1 material

template <typename VertexT>
class RenderBatch
{
private:
    VertexBuffer* VBO_;
    IndexBuffer* IBO_;
    VertexArray* VAO_;
    GLenum primitive_;
    hash_t category_;

    std::vector<VertexT> vertices_;
    std::vector<GLuint>  indices_;

public:
    explicit RenderBatch(hash_t category,
                         GLenum primitive = GL_TRIANGLES):
    VBO_(nullptr),
    IBO_(nullptr),
    VAO_(nullptr),
    primitive_(primitive),
    category_(category)
    {
        DLOGN("New render batch:", "batch");
        DLOGI("Category: <n>" + HRESOLVE(category_) + "</n>", "batch");
    }

    ~RenderBatch()
    {
        // RenderBatch objects are life-bound to their server-side counterparts
        // When they die, the OpenGL object dies as well
        DLOGN("Destroying render batch cat(<n>" + HRESOLVE(category_) + "</n>)", "batch");

        // First unbind
        if(IBO_) IBO_->unbind();
        if(VBO_) VBO_->unbind();
        if(VAO_) VAO_->unbind();

        // Release VAO
        delete VAO_;

        // Then delete buffers
        delete IBO_;
        delete VBO_;
    }

    inline uint32_t get_n_vertices() const  { return vertices_.size(); }
    inline uint32_t get_n_indices() const   { return indices_.size(); }

    void submit(Mesh<VertexT>& mesh)
    {
        uint32_t vert_offset = vertices_.size();
        mesh.set_buffer_offset(indices_.size());
        mesh.set_batch_category(category_);

        const std::vector<VertexT>& vertices = mesh.get_vertex_buffer();
        const std::vector<uint32_t>& indices = mesh.get_index_buffer();

        // Add buffer offset to indices
        std::vector<uint32_t> transformed_indices;
        transformed_indices.resize(indices.size());
        std::transform(indices.begin(), indices.end(), transformed_indices.begin(),
                       [=](uint32_t ind) -> uint32_t { return ind+vert_offset; });

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

#ifdef __DEBUG__
        size_t size_vertex_kb = (nvert * sizeof(VertexT))  / 1024;
        size_t size_index_kb  = (nind  * sizeof(uint32_t)) / 1024;
        DLOGN("Sending render batch cat(<n>" + HRESOLVE(category_) + "</n>)", "batch");
        DLOGI("#vertices: " + std::to_string(nvert) + "/"
                            + std::to_string(vertices_.size()) + " -> <v>"
                            + std::to_string(size_vertex_kb) + "kB</v>", "batch");
        DLOGI("#indices:  " + std::to_string(nind) + "/"
                            + std::to_string(indices_.size()) + " -> <v>"
                            + std::to_string(size_index_kb) + "kB</v>", "batch");
#endif

        VAO_ = VertexArray::create();
        VAO_->bind();

        VBO_ = VertexBuffer::create(reinterpret_cast<float*>(vertices_.data()), nvert*sizeof(VertexT), dynamic);

        VAO_->set_layout(ctti::type_id<VertexT>().hash());
        VAO_->unbind();

        IBO_ = IndexBuffer::create(indices_.data(), nind*sizeof(uint32_t), dynamic);
    }

    void stream(const Mesh<VertexT>& mesh, uint32_t offset=0)
    {
        VBO_->stream(reinterpret_cast<float*>(mesh.get_vertex_buffer().data()), mesh.get_nv()*sizeof(VertexT), offset);
        IBO_->stream(mesh.get_index_buffer().data(), mesh.get_ni()*sizeof(uint32_t), offset);
    }

    void draw(uint32_t n_elements, uint32_t offset) const
    {
        if(n_elements==0) return;

        VAO_->bind();
        IBO_->bind();
        glDrawElements(primitive_, dimensionality(primitive_)*n_elements, GL_UNSIGNED_INT,
                      (void*)(offset * sizeof(GLuint)));
        IBO_->unbind();
        //VAO_->unbind();
    }

    inline void draw(const BufferToken& buffer_token) const
    {
        draw(buffer_token.n_elements, buffer_token.buffer_offset);
    }
};

}

#endif // RENDER_BATCH_HPP
