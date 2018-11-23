#ifndef AABB_H
#define AABB_H

#include <vector>
#include <array>
#include <memory>

#include "math3d.h"
#include "transformation.h"

namespace wcore
{

class Model;

class OBB
{
private:
    Transformation&  parent_transform_;
    math::mat4       proper_scale_;
    math::mat4       offset_;
    math::mat4       proper_transform_;
    std::array<math::vec3, 8> vertices_;

public:
    OBB(Model& parent);
    ~OBB();

    void update();

    inline void set_offset(const math::vec3& offset) { offset_.init_translation(offset); }
    inline const std::array<math::vec3, 8>& get_vertices() const { return vertices_; }
    inline const math::mat4& get_model_matrix() const          { return proper_transform_; }

};

class AABB
{
private:
    Transformation&  parent_transform_;
    math::mat4       proper_scale_;
    math::mat4       offset_;
    math::mat4       proper_transform_;
    std::array<math::vec3, 8> vertices_;
    float            extent_[6];

public:
    AABB(Model& parent);
    ~AABB();

    void update();
    bool is_inside(const math::vec3& point) const;

    inline float xmin() const { return extent_[0]; }
    inline float xmax() const { return extent_[1]; }
    inline float ymin() const { return extent_[2]; }
    inline float ymax() const { return extent_[3]; }
    inline float zmin() const { return extent_[4]; }
    inline float zmax() const { return extent_[5]; }

    inline void set_offset(const math::vec3& offset) { offset_.init_translation(offset); }
    inline const std::array<math::vec3, 8>& get_vertices() const { return vertices_; }
    inline const math::mat4& get_model_matrix() const          { return proper_transform_; }
};

class Camera;
class FrustumBox
{
private:
    std::array<math::vec3, 8> vertices_;
    std::array<math::vec3, 6> normals_;
    std::vector<float> splits_;
    float extent_[6];
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
    template <typename BB> bool collides(const BB& bb);
    bool collides_sphere(const math::vec3& center, float radius) const;

    math::vec3 split_center(uint32_t splitIndex) const;

private:
    inline float dist_to_plane(uint32_t index, math::vec3 vPoint) const;
};

inline float FrustumBox::dist_to_plane(uint32_t index, math::vec3 point) const
{
    return math::dot(point-vertices_[planePoints[index]], normals_[index]);
}

template <typename BB>
bool FrustumBox::collides(const BB& bb)
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

}

#endif // AABB_H
