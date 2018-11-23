#include <unordered_set>

#include "surface_mesh.h"
#include "vertex_format.h"

namespace wcore
{

void FaceMesh::build_normals()
{
    if(indices_.size()==0)
        return;
    for(size_t ii=0; ii+2<indices_.size(); ii+=3)
    {
        // For each triangle in indices list
        const math::vec3& p1 = vertices_[indices_[ii+0]].get_position();
        const math::vec3& p2 = vertices_[indices_[ii+1]].get_position();
        const math::vec3& p3 = vertices_[indices_[ii+2]].get_position();

        // Compute local normal using cross product
        math::vec3 U(p2-p1);
        math::vec3 V(p3-p1);
        math::vec3 normal = math::normalize(math::cross(U,V));

        // Assign normal to each vertex
        vertices_[indices_[ii+0]].set_normal(normal);
        vertices_[indices_[ii+1]].set_normal(normal);
        vertices_[indices_[ii+2]].set_normal(normal);
    }
}

void FaceMesh::build_normals_and_tangents()
{
    if(indices_.size()==0)
        return;
    // For each triangle in indices list
    for(size_t ii=0; ii+2<indices_.size(); ii+=3)
    {
        // Compute normals
        // Get positions
        const math::vec3& p1 = vertices_[indices_[ii+0]].get_position();
        const math::vec3& p2 = vertices_[indices_[ii+1]].get_position();
        const math::vec3& p3 = vertices_[indices_[ii+2]].get_position();

        // Compute local normal using cross product
        math::vec3 e1(p2-p1);
        math::vec3 e2(p3-p1);
        math::vec3 normal = math::normalize(math::cross(e1,e2));

        // Assign normal to each vertex
        vertices_[indices_[ii+0]].set_normal(normal);
        vertices_[indices_[ii+1]].set_normal(normal);
        vertices_[indices_[ii+2]].set_normal(normal);

        // Compute tangents
        // Get UVs and compute deltas
        const math::vec2& uv1 = vertices_[indices_[ii+0]].get_uv();
        const math::vec2& uv2 = vertices_[indices_[ii+1]].get_uv();
        const math::vec2& uv3 = vertices_[indices_[ii+2]].get_uv();
        math::vec2 deltaUV1(uv2-uv1);
        math::vec2 deltaUV2(uv3-uv1);

        float det_inv = 1.0f/(deltaUV1.x()*deltaUV2.y() - deltaUV2.x()*deltaUV1.y());
        math::vec3 tangent(e1*deltaUV2.y() - e2*deltaUV1.y());
        tangent *= det_inv;

        // Assign tangent to each vertex
        vertices_[indices_[ii+0]].set_tangent(tangent);
        vertices_[indices_[ii+1]].set_tangent(tangent);
        vertices_[indices_[ii+2]].set_tangent(tangent);
    }
}

void FaceMesh::smooth_normals(SmoothFunc Func)
{
    std::unordered_set<uint32_t> checked; // Keeps track of checked positions

    // For each vertex
    for(size_t ii=0; ii<vertices_.size(); ++ii)
    {
        // Get vertex position
        const math::vec3& position = vertices_[ii].get_position();

        // Compute position hash
        uint32_t pos_hash = std::hash<math::vec3>()(position);


        // Check that we haven't already traversed the corresponding position class,
        // and if so, continue
        if(checked.find(pos_hash) != checked.end()) continue;
        checked.insert(pos_hash);

        // Traverse position class
        traverse_position_class(position, [&](IndexRange range)
        {
            math::vec3 normal0(0,0,0); // This will become the mean normal at current position
            //for(auto vert : common_vertices)
            for(auto it = range.first; it != range.second; ++it)
            {
                // Sum up all normals
                normal0 += vertices_[it->second].get_normal();
            }
            normal0.normalize();
            for(auto it = range.first; it != range.second; ++it)
            {
                const math::vec3& normal_i = vertices_[it->second].get_normal();
                float alpha = Func(math::dot(normal_i, normal0));
                math::vec3 new_normal(math::lerp(normal_i, normal0, alpha));
                // Update normal for each vertex from the list
                vertices_[it->second].set_normal(new_normal);
            }
            //The new normal value for V is the mean of these triangles' normals
        });
    }
}

void FaceMesh::smooth_normals_and_tangents(SmoothFunc Func)
{
    std::unordered_set<uint32_t> checked; // Keeps track of checked positions

    // For each vertex
    for(size_t ii=0; ii<vertices_.size(); ++ii)
    {
        // Get vertex position
        const math::vec3& position = vertices_[ii].get_position();

        // Compute position hash
        uint32_t pos_hash = std::hash<math::vec3>()(position);


        // Check that we haven't already traversed the corresponding position class,
        // and if so, continue
        if(checked.find(pos_hash) != checked.end()) continue;
        checked.insert(pos_hash);

        // Traverse position class
        traverse_position_class(position, [&](IndexRange range)
        {
            math::vec3 normal0(0,0,0);  // This will become the mean normal at current position
            math::vec3 tangent0(0,0,0); // This will become the mean tangent at current position
            for(auto it = range.first; it != range.second; ++it)
            {
                auto&& vert = vertices_[it->second];
                // Sum up all normals
                normal0  += vert.get_normal();
                tangent0 += vert.get_tangent();
            }
            normal0.normalize();
            tangent0.normalize();
            for(auto it = range.first; it != range.second; ++it)
            {
                auto&& vert = vertices_[it->second];
                // Smooth normal
                const math::vec3& normal_i  = vert.get_normal();
                float alpha = Func(math::dot(normal_i, normal0));
                math::vec3 new_normal(math::lerp(normal_i, normal0, alpha));

                // Update normal for each vertex from the list
                vert.set_normal(new_normal);

                // Smooth tangent
                const math::vec3& tangent_i = vert.get_tangent();
                alpha = Func(math::dot(tangent_i, tangent0));
                math::vec3 new_tangent(math::lerp(tangent_i, tangent0, alpha));
                // Update tangent for each vertex from the list
                vert.set_tangent(new_tangent);
            }
        });
    }
}


void TriangularMesh::build_normals()
{
    // For each vertex
    for(uint32_t ii=0; ii<vertices_.size(); ++ii)
    {
        math::vec3 normal0(0);
        // For each triangle that contain this vertex
        traverse_triangle_class(ii, [&](TriangleRange range)
        {
            for(auto it = range.first; it != range.second; ++it)
            {
                uint32_t tri_index = it->second;
                // Get positions
                const math::vec3& p1 = vertices_[indices_[tri_index+0]].get_position();
                const math::vec3& p2 = vertices_[indices_[tri_index+1]].get_position();
                const math::vec3& p3 = vertices_[indices_[tri_index+2]].get_position();
                // Compute local normal using cross product
                math::vec3 e1(p2-p1);
                math::vec3 e2(p3-p1);
                math::vec3 normal = math::normalize(math::cross(e1,e2));
                normal0 += normal;
            }
        });

        // Assign mean normal to vertex
        vertices_[ii].set_normal(normal0.normalized());
    }
}

void TriangularMesh::build_normals_and_tangents()
{
    // For each vertex
    for(uint32_t ii=0; ii<vertices_.size(); ++ii)
    {
        math::vec3 normal0(0);
        math::vec3 tangent0(0);
        // For each triangle that contain this vertex
        traverse_triangle_class(ii, [&](TriangleRange range)
        {
            for(auto it = range.first; it != range.second; ++it)
            {
                uint32_t tri_index = it->second;

                // Get positions
                const math::vec3& p1 = vertices_[indices_[tri_index+0]].get_position();
                const math::vec3& p2 = vertices_[indices_[tri_index+1]].get_position();
                const math::vec3& p3 = vertices_[indices_[tri_index+2]].get_position();
                // Compute local normal using cross product
                math::vec3 e1(p2-p1);
                math::vec3 e2(p3-p1);
                math::vec3 normal = math::normalize(math::cross(e1,e2));
                normal0 += normal;

                // Get UVs
                const math::vec2& uv1 = vertices_[indices_[tri_index+0]].get_uv();
                const math::vec2& uv2 = vertices_[indices_[tri_index+1]].get_uv();
                const math::vec2& uv3 = vertices_[indices_[tri_index+2]].get_uv();
                // Compute deltas
                math::vec2 deltaUV1(uv2-uv1);
                math::vec2 deltaUV2(uv3-uv1);
                // Compute local tangent
                float det_inv = 1.0f/(deltaUV1.x()*deltaUV2.y() - deltaUV2.x()*deltaUV1.y());
                math::vec3 tangent(e1*deltaUV2.y() - e2*deltaUV1.y());
                tangent *= det_inv;
                tangent0 += tangent;
            }
        });

        // Assign mean normal to vertex
        vertices_[ii].set_normal(normal0.normalized());
        vertices_[ii].set_tangent(tangent0.normalized());
    }
}

}
