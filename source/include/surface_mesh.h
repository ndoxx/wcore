#ifndef SURFACE_MESH_H
#define SURFACE_MESH_H

#include <functional>

#include "mesh.hpp"
#include "vertex_format.h"
#include "logger.h"

namespace wcore
{

typedef std::function<float(float)> SmoothFunc;
static SmoothFunc SMOOTH_MAX                 = [](float x){ (void)x; return 1.0f; };
static SmoothFunc SMOOTH_HEAVISIDE           = [](float x){ x=1-fabs(x); return (x<0.1f)?1.0f:0.0f; };
static SmoothFunc SMOOTH_LINEAR              = [](float x){ x=1-fabs(x); return 1.0f-x; };
static SmoothFunc SMOOTH_COMPRESS_LINEAR     = [](float x){ x=1-fabs(x); return (x<0.5f)?1.0f:-2.0f*x+2.0f; };
static SmoothFunc SMOOTH_COMPRESS_QUADRATIC  = [](float x){ x=1-x; float a=0.75f;  return a*x*x-(a+1.0f)*x+1.0f; };

class FaceMesh: public SurfaceMesh
{
private:
    typedef std::unordered_multimap<math::vec3, uint32_t> VertexHashMap;
    typedef std::pair<VertexHashMap::iterator, VertexHashMap::iterator> IndexRange;
    VertexHashMap position_classes_;

public:
    FaceMesh(): SurfaceMesh(){}
    virtual ~FaceMesh() {}

    inline void set_vertex(uint32_t index, const Vertex3P3N3T2U& vertex)
    {
        assert(index<get_nv() && "Index out of bounds during vertex assignment operation.");
        // Remove old position class association
        IndexRange range = position_classes_.equal_range(vertices_[index].get_position());
        for(auto it = range.first; it != range.second; ++it)
        {
            if(it->second == index)
            {
                position_classes_.erase(it);
                break;
            }
        }
        // Update vertex
        _set_vertex(index, vertex);
        // Associate position to this index
        position_classes_.insert(VertexHashMap::value_type(vertex.get_position(),index));
    }

    template <typename... Args>
    inline size_t emplace_vertex(Args&&... args)
    {
        size_t index = _emplace_vertex(std::forward<Args>(args)...);
        position_classes_.insert(VertexHashMap::value_type(vertices_.back().get_position(),vertices_.size()-1));
        return index;
    }

    inline void push_triangle(uint32_t T1, uint32_t T2, uint32_t T3)
    {
        _push_triangle(T1, T2, T3);
    }

    // index position class traversal
    // visit a range of indices corresponding to vertices at the same position
    inline void traverse_position_class(const math::vec3& position, std::function<void(IndexRange)> visitor)
    {
        visitor(position_classes_.equal_range(position));
    }

    virtual void build_normals() override;
    virtual void build_normals_and_tangents() override;

    void smooth_normals(SmoothFunc Func = SMOOTH_MAX);
    void smooth_normals_and_tangents(SmoothFunc Func = SMOOTH_MAX);
};

class TriangularMesh: public SurfaceMesh
{
private:
    typedef std::multimap<uint32_t, uint32_t> TriangleMap;
    typedef std::pair<TriangleMap::iterator, TriangleMap::iterator> TriangleRange;
    TriangleMap triangle_classes_;

public:
    TriangularMesh(): SurfaceMesh(){}
    virtual ~TriangularMesh() {}

    inline void set_triangle_by_index(uint32_t tri_index, const math::i32vec3& T)
    {
        assert(tri_index+2<get_ni() && "Index out of bounds during triangle assignment operation.");
        assert(tri_index%3 == 0   && "Index is not a triangle index (index%3 != 0)");
        for(uint32_t ii=0; ii<3; ++ii)
        {
            // Remove old triangle class association
            TriangleRange range = triangle_classes_.equal_range(indices_[tri_index+ii]);
            for(auto it = range.first; it != range.second; ++it)
            {
                if(it->second == tri_index)
                {
                    triangle_classes_.erase(it);
                    break;
                }
            }

            // Update triangle with new indices
            indices_[tri_index+ii] = T[ii];
            // Associate each vertex position to the index of triangle
            triangle_classes_.insert(TriangleMap::value_type(T[ii], tri_index));
        }
    }

    template <typename... Args>
    inline size_t emplace_vertex(Args&&... args)
    {
        return _emplace_vertex(std::forward<Args>(args)...);
    }

    inline void push_triangle(uint32_t T1, uint32_t T2, uint32_t T3)
    {
        uint32_t cur_index = indices_.size();
        _push_triangle(T1, T2, T3);
        // Associate each vertex position to the index of triangle
        triangle_classes_.insert(TriangleMap::value_type(T1, cur_index));
        triangle_classes_.insert(TriangleMap::value_type(T2, cur_index));
        triangle_classes_.insert(TriangleMap::value_type(T3, cur_index));
    }

    inline void push_triangle(const math::i32vec3& T)
    {
        push_triangle(T.x(), T.y(), T.z());
    }

    inline math::vec3 mid_position(uint32_t P1, uint32_t P2)
    {
        return math::lerp(vertices_.at(P1).get_position(), vertices_.at(P2).get_position(), 0.5f);
    }

    inline math::vec2 mid_uv(uint32_t P1, uint32_t P2)
    {
        return math::lerp(vertices_.at(P1).get_uv(), vertices_.at(P2).get_uv(), 0.5f);
    }

    // index triangle class traversal
    // visit a range of indices corresponding to triangles that contain a vertex at given position
    inline void traverse_triangle_class(uint32_t index, std::function<void(TriangleRange)> visitor)
    {
        visitor(triangle_classes_.equal_range(index));
    }

    virtual void build_normals() override;
    virtual void build_normals_and_tangents() override;
};

}

#endif // SURFACE_MESH_H
