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
class BufferUnit;

template <typename VertexT>
class Mesh
{
    friend class BufferUnit<VertexT>;

protected:
    std::vector<VertexT>  vertices_;
    std::vector<uint32_t> indices_;
    std::array<float, 6>  dimensions_;
    uint32_t              buffer_offset_;
    uint32_t              n_elements_;

public:
    Mesh():
    buffer_offset_(0),
    n_elements_(0){}

    Mesh(Mesh&& other) noexcept
    : vertices_(std::move(other.vertices_))
    , indices_(std::move(other.indices_))
    , dimensions_(other.dimensions_)
    , buffer_offset_(other.buffer_offset_)
    , n_elements_(other.n_elements_){}

    Mesh(const Mesh& other)
    : vertices_(other.vertices_)
    , indices_(other.indices_)
    , dimensions_(other.dimensions_)
    , buffer_offset_(other.buffer_offset_)
    , n_elements_(other.n_elements_) {}

    virtual ~Mesh() {}

    Mesh& operator= (const Mesh& other)
    {
        vertices_ = other.vertices_;
        indices_ = other.indices_;
        buffer_offset_ = other.buffer_offset_;
        n_elements_ = other.n_elements_;
        dimensions_ = other.dimensions_;
        return *this;
    }

    inline uint32_t get_nv() const            { return vertices_.size(); }
    inline uint32_t get_ni() const            { return indices_.size(); }
    inline uint32_t get_n_elements() const    { return n_elements_; }
    inline uint32_t get_buffer_offset() const { return buffer_offset_; }

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
    template <typename... Args>
    inline size_t _emplace_vertex(Args&&... args)
    {
        vertices_.emplace_back(std::forward<Args>(args)...);
        return vertices_.size()-1;
    }

    inline void _push_triangle(uint32_t T1, uint32_t T2, uint32_t T3)
    {
        indices_.push_back(T1);
        indices_.push_back(T2);
        indices_.push_back(T3);
        ++n_elements_;
    }

    inline void push_line(uint32_t P1, uint32_t P2)
    {
        indices_.push_back(P1);
        indices_.push_back(P2);
        ++n_elements_;
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
    inline void set_buffer_offset(uint32_t offset) { buffer_offset_ = offset; }
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
