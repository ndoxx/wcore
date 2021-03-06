#ifndef MESH_H
#define MESH_H

#include <vector>
#include <array>
#include <limits>
#include <functional>

#ifdef __DEBUG__
    #include <iostream>
#endif

#include "math3d.h"

namespace wcore
{

template <typename VertexT>
class RenderBatch;

struct BufferToken
{
    hash_t   batch_category = ""_h;
    uint32_t buffer_offset  = 0;
    uint32_t n_elements     = 0;
};

template <typename VertexT>
class Mesh
{
    friend class RenderBatch<VertexT>;

protected:
    std::vector<VertexT>  vertices_;
    std::vector<uint32_t> indices_;
    std::array<float, 6>  dimensions_;
    BufferToken           buffer_token_;
    bool                  centered_;

public:
    Mesh():
    centered_(false){}

    Mesh(std::vector<VertexT>&& vertices,
         std::vector<uint32_t>&& indices,
         int dimensionality=3):
    vertices_(std::move(vertices)),
    indices_(std::move(indices)),
    centered_(false)
    {
        buffer_token_.n_elements = indices_.size() / dimensionality;
        compute_dimensions();
    }

    virtual ~Mesh() {}

    inline uint32_t get_nv() const            { return vertices_.size(); }
    inline uint32_t get_ni() const            { return indices_.size(); }
    inline uint32_t get_n_elements() const    { return buffer_token_.n_elements; }
    inline uint32_t get_buffer_offset() const { return buffer_token_.buffer_offset; }
    inline const BufferToken& get_buffer_token() const { return buffer_token_; }

    inline void set_batch_category(hash_t category) { buffer_token_.batch_category = category; }

    inline bool is_centered() const           { return centered_; }
    inline void set_centered(bool value)      { centered_ = value; }

    inline const std::vector<VertexT>&  get_vertex_buffer() const { return vertices_; }
    inline const std::vector<uint32_t>& get_index_buffer()  const { return indices_; }
    inline const std::array<float, 6>& get_dimensions() const     { return dimensions_; }


    inline void _set_vertex(uint32_t index, const VertexT& vertex)
    {
        vertices_[index] = vertex;
    }
    template <typename... Args>
    inline void set_vertex(uint32_t index, Args&&... args)
    {
        _set_vertex(index, VertexT(std::forward<Args>(args)...));
    }

    inline size_t _push_vertex(VertexT&& vertex)
    {
        vertices_.push_back(std::forward<VertexT>(vertex));
        return vertices_.size()-1;
    }

    inline void _push_triangle(uint32_t T1, uint32_t T2, uint32_t T3)
    {
        indices_.push_back(T1);
        indices_.push_back(T2);
        indices_.push_back(T3);
        ++buffer_token_.n_elements;
    }

    inline void push_line(uint32_t P1, uint32_t P2)
    {
        indices_.push_back(P1);
        indices_.push_back(P2);
        ++buffer_token_.n_elements;
    }

    inline const VertexT& operator[](uint32_t index) const
    {
        assert(index<get_nv() && "Mesh.operator[] -> Index out of bounds.");
        return vertices_[index];
    }

    inline VertexT& operator[](uint32_t index)
    {
        assert(index<get_nv() && "Mesh.operator[] -> Index out of bounds.");
        return vertices_[index];
    }

    inline const VertexT& at(uint32_t index) const
    {
        assert(index<get_nv() && "Mesh.operator[] -> Index out of bounds.");
        return vertices_[index];
    }

    // Visitors
    void traverse_vertices(std::function<void(const VertexT&)> visitor)
    {
        for(const VertexT& vertex : vertices_)
            visitor(vertex);
    }

    void traverse_vertices_mut(std::function<void(VertexT&)> visitor)
    {
        for(VertexT& vertex : vertices_)
            visitor(vertex);
    }

    void traverse_triangles(std::function<void(uint32_t,uint32_t,uint32_t)> visitor)
    {
        for(size_t ii=0; ii+2<indices_.size(); ii+=3)
            visitor(indices_[ii+0], indices_[ii+1], indices_[ii+2]);
    }

    virtual void build_normals(){}
    virtual void build_tangents(){}
    virtual void build_normals_and_tangents(){}

    void compute_dimensions()
    {
        float xmin=std::numeric_limits<float>::max();
        float xmax=-std::numeric_limits<float>::max();
        float ymin=std::numeric_limits<float>::max();
        float ymax=-std::numeric_limits<float>::max();
        float zmin=std::numeric_limits<float>::max();
        float zmax=-std::numeric_limits<float>::max();
        traverse_vertices([&](const VertexT& vertex)
        {
            const math::vec3& pos = vertex.position_;

            xmin = fmin(xmin, pos.x());
            xmax = fmax(xmax, pos.x());
            ymin = fmin(ymin, pos.y());
            ymax = fmax(ymax, pos.y());
            zmin = fmin(zmin, pos.z());
            zmax = fmax(zmax, pos.z());
        });
        dimensions_ = {xmin, xmax, ymin, ymax, zmin, zmax};
    }

#ifdef __DEBUG__
    void dbg_display_dimensions()
    {
        std::cout << "xmin: " << dimensions_[0] << " xmax: " << dimensions_[1] << std::endl;
        std::cout << "ymin: " << dimensions_[2] << " ymax: " << dimensions_[3] << std::endl;
        std::cout << "zmin: " << dimensions_[4] << " zmax: " << dimensions_[5] << std::endl;
    }
#endif

private:
    inline void set_buffer_offset(uint32_t offset) { buffer_token_.buffer_offset = offset; }
};

struct Vertex3P3N3T2U;
struct Vertex3P2U;
struct Vertex3P3C;
struct Vertex3P;
using SurfaceMesh = Mesh<Vertex3P3N3T2U>;
using MeshPU = Mesh<Vertex3P2U>;
using MeshPC = Mesh<Vertex3P3C>;
using MeshP = Mesh<Vertex3P>;

}

#endif // MESH_H
