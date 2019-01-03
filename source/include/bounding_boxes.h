#ifndef AABB_H
#define AABB_H

#include <vector>
#include <array>
#include <memory>
#include <cassert>

#include "math3d.h"

namespace wcore
{

class Model;
struct Ray;

class AABB;
struct BoundingRegion
{
    BoundingRegion() = default;
    explicit BoundingRegion(math::extent_t&& value);
    explicit BoundingRegion(const math::extent_t& value);

    void update();
    bool intersects(const BoundingRegion& other) const;
    bool intersects(const math::vec3& point) const;

    bool contains(const BoundingRegion& other) const;
    bool contains(const AABB& aabb) const;
    // A point is contained if it intersects bounding region
    inline bool contains(const math::vec3& point) const { return intersects(point); }

    std::array<math::vec3, 8> get_vertices() const;

    math::extent_t extent;
    math::vec3 mid_point;
    math::vec3 half;
};

class OBB
{
private:
    BoundingRegion   bounding_region_; // Bounding region in model space
    math::mat4       proper_scale_;
    math::mat4       proper_transform_;
    math::vec3       offset_;
    std::array<math::vec3, 8> vertices_;

public:
    OBB(const math::extent_t& parent_extent, bool centered);
    ~OBB();

    void update(const math::mat4& parent_model_matrix);

    inline void set_offset(const math::vec3& offset)             { offset_ = offset; }
    inline const std::array<math::vec3, 8>& get_vertices() const { return vertices_; }
    inline const math::mat4& get_model_matrix() const            { return proper_transform_; }
    inline const math::extent_t& get_extent() const              { return bounding_region_.extent; }
};

class AABB
{
private:
    BoundingRegion   bounding_region_; // Bounding region in world space
    math::mat4       proper_transform_;
    std::array<math::vec3, 8> vertices_;

public:
    AABB();
    ~AABB();

    void update(const OBB& parent_OBB);
    bool is_inside(const math::vec3& point) const;

    inline float xmin() const { return bounding_region_.extent[0]; }
    inline float xmax() const { return bounding_region_.extent[1]; }
    inline float ymin() const { return bounding_region_.extent[2]; }
    inline float ymax() const { return bounding_region_.extent[3]; }
    inline float zmin() const { return bounding_region_.extent[4]; }
    inline float zmax() const { return bounding_region_.extent[5]; }

    inline float extent(uint32_t index) const
    {
        assert(index<6 && "[AABB] extent() index out of bounds.");
        return bounding_region_.extent[index];
    }
    inline const BoundingRegion& get_bounding_region() const { return bounding_region_; }
    inline const math::extent_t& get_extent() const          { return bounding_region_.extent; }

    inline const std::array<math::vec3, 8>& get_vertices() const { return vertices_; }
    inline const math::mat4& get_model_matrix() const            { return proper_transform_; }
};

class Camera;
class FrustumBox
{
private:
    std::array<math::vec3, 8> vertices_;
    std::array<math::vec3, 6> normals_;
    std::vector<float> splits_;
    math::extent_t extent_;
    static const std::vector<uint32_t> planePoints;

public:
    FrustumBox();
    ~FrustumBox();

    inline const math::vec3& RBN() const { return vertices_[0]; }
    inline const math::vec3& RBF() const { return vertices_[1]; }
    inline const math::vec3& LBF() const { return vertices_[2]; }
    inline const math::vec3& LBN() const { return vertices_[3]; }
    inline const math::vec3& RTN() const { return vertices_[4]; }
    inline const math::vec3& RTF() const { return vertices_[5]; }
    inline const math::vec3& LTF() const { return vertices_[6]; }
    inline const math::vec3& LTN() const { return vertices_[7]; }

    // Normals point to the interior of frustum
    inline math::vec3 normal_L() const { return math::cross(LBF()-LBN(), LTN()-LBN()).normalized(); } // With point 3
    inline math::vec3 normal_R() const { return math::cross(RTN()-RBN(), RBF()-RBN()).normalized(); } // With point 0
    inline math::vec3 normal_B() const { return math::cross(RBN()-LBN(), LBF()-LBN()).normalized(); } // With point 3
    inline math::vec3 normal_T() const { return math::cross(LTF()-LTN(), RTN()-LTN()).normalized(); } // With point 7
    inline math::vec3 normal_N() const { return math::cross(LTN()-LBN(), RBN()-LBN()).normalized(); } // With point 3
    inline math::vec3 normal_F() const { return math::cross(RBF()-LBF(), LTF()-LBF()).normalized(); } // With point 2

    inline const std::array<math::vec3, 8>& get_corners() const { return vertices_; }
    inline const std::array<math::vec3, 6>& get_normals() const { return normals_; }

    void update(const Camera& camera);
    template <typename BB> bool intersects(const BB& bb) const;
    bool collides_sphere(const math::vec3& center, float radius) const;
    bool intersects(const math::vec3& point) const;

    math::vec3 split_center(uint32_t splitIndex) const;

private:
    inline float dist_to_plane(uint32_t index, math::vec3 vPoint) const;
};

inline float FrustumBox::dist_to_plane(uint32_t index, math::vec3 point) const
{
    return math::dot(point-vertices_[planePoints[index]], normals_[index]);
}

template <typename BB>
bool FrustumBox::intersects(const BB& bb) const
{
    const std::array<math::vec3, 8>& verts = bb.get_vertices();

    // For each frustum plane
    for(uint32_t ii=0; ii<6; ++ii)
    {
        // Bounding box is considered outside iif all its vertices are below the SAME plane
        bool all_out = true;
        for(auto p: verts)
        {
            // Check if point is above plane
            if(dist_to_plane(ii, p)>0)
            {
                all_out = false;
                break;
            }
        }
        if(all_out)
            return false;
    }
    return true;
}

struct RayCollisionData
{
    float near = 0.0f;
    float far  = 0.0f;
};

bool ray_collides_extent(const Ray& ray, const math::extent_t& extent, RayCollisionData& data);
inline bool ray_collides_AABB(const Ray& ray, const AABB& aabb, RayCollisionData& data)
{
    return ray_collides_extent(ray, aabb.get_extent(), data);
}
bool ray_collides_OBB(const Ray& ray, std::shared_ptr<Model> model, RayCollisionData& data);

}

#endif // AABB_H
