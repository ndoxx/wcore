#include "bounding_boxes.h"
#include "model.h"
#include "logger.h"
#include "camera.h"
#include "algorithms.h"
#include "ray.h"

namespace wcore
{

using namespace math;

static const std::vector<vec3> CUBE_VERTICES
{
    vec3( 0.5f, 0.0f, 0.5f),    // 0
    vec3( 0.5f, 0.0f, -0.5f),   // 1
    vec3( -0.5f, 0.0f, -0.5f),  // 2
    vec3( -0.5f, 0.0f, 0.5f),   // 3
    vec3( 0.5f,  1.0f, 0.5f),   // 4
    vec3( 0.5f,  1.0f, -0.5f),  // 5
    vec3( -0.5f, 1.0f, -0.5f),  // 6
    vec3( -0.5f, 1.0f, 0.5f)    // 7
};

static const std::vector<vec3> CENTERED_CUBE_VERTICES
{
    vec3( 0.5f, -0.5f, 0.5f),   // 0
    vec3( 0.5f, -0.5f, -0.5f),  // 1
    vec3( -0.5f, -0.5f, -0.5f), // 2
    vec3( -0.5f, -0.5f, 0.5f),  // 3
    vec3( 0.5f,  0.5f, 0.5f),   // 4
    vec3( 0.5f,  0.5f, -0.5f),  // 5
    vec3( -0.5f, 0.5f, -0.5f),  // 6
    vec3( -0.5f, 0.5f, 0.5f)    // 7
};

static const std::vector<vec3> NDC_CUBE_VERTICES
{
    vec3( 1.0f, 0.0f, 1.0f),    // 0
    vec3( 1.0f, 0.0f, -1.0f),   // 1
    vec3( -1.0f, 0.0f, -1.0f),  // 2
    vec3( -1.0f, 0.0f, 1.0f),   // 3
    vec3( 1.0f,  1.0f, 1.0f),   // 4
    vec3( 1.0f,  1.0f, -1.0f),  // 5
    vec3( -1.0f, 1.0f, -1.0f),  // 6
    vec3( -1.0f, 1.0f, 1.0f)    // 7
};

BoundingRegion::BoundingRegion(math::extent_t&& value):
extent(std::move(value))
{
    update();
}

BoundingRegion::BoundingRegion(const math::extent_t& value):
extent(value)
{
    update();
}

BoundingRegion::BoundingRegion(const math::vec3& mid_point, const math::vec3& half):
mid_point(mid_point),
half(half)
{
    // Compute extent
    // [TODO] optimize this
    math::vec3 coords_min(mid_point-half);
    math::vec3 coords_max(mid_point+half);
    extent[0] = coords_min.x();
    extent[2] = coords_min.y();
    extent[4] = coords_min.z();
    extent[1] = coords_max.x();
    extent[3] = coords_max.y();
    extent[5] = coords_max.z();
}


void BoundingRegion::update()
{
    mid_point = math::vec3((extent[1]+extent[0])*0.5f,
                           (extent[3]+extent[2])*0.5f,
                           (extent[5]+extent[4])*0.5f);
    half = math::vec3((extent[1]-extent[0])*0.5f,
                      (extent[3]-extent[2])*0.5f,
                      (extent[5]-extent[4])*0.5f);
}

std::array<math::vec3, 8> BoundingRegion::get_vertices() const
{
    return std::array<math::vec3, 8>
    ({
        vec3(extent[1], extent[2], extent[5]),   // 0
        vec3(extent[1], extent[2], extent[4]),   // 1
        vec3(extent[0], extent[2], extent[4]),   // 2
        vec3(extent[0], extent[2], extent[5]),   // 3
        vec3(extent[1], extent[3], extent[5]),   // 4
        vec3(extent[1], extent[3], extent[4]),   // 5
        vec3(extent[0], extent[3], extent[4]),   // 6
        vec3(extent[0], extent[3], extent[5])    // 7
    });
}

OBB::OBB(const math::extent_t& parent_extent, bool centered):
bounding_region_(parent_extent)
{
    // Initialize proper scale (non uniform) to parent's intrinsic scale
    proper_scale_.init_scale(2.0f*bounding_region_.half);

    // Translate OBB if mesh not centered bc we use the vertices of a centered cube in update()
    if(!centered)
        offset_ = bounding_region_.mid_point;
}

OBB::~OBB() = default;

void OBB::update(const math::mat4& parent_model_matrix)
{
    // Compute OBB transform from parent transform
    proper_transform_ = proper_scale_;
    math::translate_matrix(proper_transform_, offset_);
    proper_transform_ = parent_model_matrix * proper_transform_;

    // Compute OBB vertices
    for(uint32_t ii=0; ii<8; ++ii)
        vertices_[ii] = proper_transform_ * CENTERED_CUBE_VERTICES[ii];
}


AABB::AABB()
{

}

AABB::~AABB() = default;

void AABB::update(const OBB& parent_OBB)
{
    // Get parent OBB and compute its extent in world space
    math::extent_t extent;
    math::compute_extent(parent_OBB.get_vertices(), extent);
    bounding_region_ = BoundingRegion(extent);

    // Create AABB from OBB extent
    proper_transform_.init_scale(2.0f*bounding_region_.half);
    math::translate_matrix(proper_transform_, bounding_region_.mid_point);

    // Compute AABB vertices
    for(uint32_t ii=0; ii<8; ++ii)
        vertices_[ii] = proper_transform_ * CENTERED_CUBE_VERTICES[ii];
}

bool AABB::is_inside(const vec3& point) const
{
    if(point.x()>xmin() && point.x()<xmax() &&
       point.y()>ymin() && point.y()<ymax() &&
       point.z()>zmin() && point.z()<zmax())
        return true;
    return false;
}


const std::vector<uint32_t> FrustumBox::planePoints = {3, 0, 3, 7, 3, 2};

FrustumBox::FrustumBox()
{
    // For shadowmap frustum splits, unused atm.
    float lambda = 0.5f;
    float near = 0.1f;
    float far = 100.0f;
    uint32_t m = 4;
    for(uint32_t ii=0; ii<m; ++ii)
    {
        float cLog = near * pow(far/near, ii/(1.0f*(m-1)))/far - near/far;
        float cUni = ii/(1.0f*(m-1));
        float cii  = lambda * cLog + (1.0f-lambda) * cUni;
        splits_.push_back(cii);
    }
}

FrustumBox::~FrustumBox()
{

}

void FrustumBox::update(const Camera& camera)
{
    const vec3& right   = camera.get_right();
    const vec3& up      = camera.get_up();
    const vec3& forward = -camera.get_forward();
    // camera forward is towards the negative z direction
    // we negate it here because world coordinates use
    // the opposite convention (z axis towards the positive z values)

    const vec3& p    = camera.get_position();
    const Frustum& f = camera.get_frustum();

    // Near plane dimensions
    float hnear = f.h;
    float wnear = f.w;
    float ratio = f.w/f.h;

    // Far plane dimensions
    float hfar = f.h/f.n * f.f;
    float wfar = hfar*ratio;

    // Far plane center
    vec3 fc(p + forward*f.f);
    // Near plane center
    vec3 nc(p + forward*f.n);

    // Intermediary vectors
    vec3 up_near(up * (hnear * 0.5f));
    vec3 up_far (up * (hfar * 0.5f));
    vec3 right_near(right * (wnear * 0.5f));
    vec3 right_far (right * (wfar * 0.5f));

    // Now compute vertices in world space using vector addition
    vertices_[0] = nc - up_near + right_near; // RBN
    vertices_[1] = fc - up_far  + right_far;  // RBF
    vertices_[2] = fc - up_far  - right_far;  // LBF
    vertices_[3] = nc - up_near - right_near; // LBN
    vertices_[4] = nc + up_near + right_near; // RTN
    vertices_[5] = fc + up_far  + right_far;  // RTF
    vertices_[6] = fc + up_far  - right_far;  // LTF
    vertices_[7] = nc + up_near - right_near; // LTN

    // Compute axis-aligned bounds
    math::compute_extent(vertices_, extent_);

    // Compute the normals
    normals_[0] = normal_L();
    normals_[1] = normal_R();
    normals_[2] = normal_B();
    normals_[3] = normal_T();
    normals_[4] = normal_N();
    normals_[5] = normal_F();
}

math::vec3 FrustumBox::split_center(uint32_t splitIndex) const
{
    splitIndex = fmax(splitIndex, splits_.size()-2);
    //vec3 RBii = lerp(RBN(),RBF(),splits_[splitIndex]);
    vec3 LBii = lerp(LBN(),LBF(),splits_[splitIndex]);
    vec3 RBii1 = lerp(RBN(),RBF(),splits_[splitIndex+1]);
    //vec3 LBii1 = lerp(LBN(),LBF(),splits_[splitIndex+1]);
    return lerp(LBii,RBii1,0.5);
}

static const float epsilon = 0.0001f;

// Scott Owen's algo for ray/AABB intersection
// adapted from pseudo-code in: https://www.siggraph.org//education/materials/HyperGraph/raytrace/rtinter3.htm
bool ray_collides_extent(const Ray& ray, const extent_t& extent, RayCollisionData& data)
{
    float Tnear = -std::numeric_limits<float>::max();
    float Tfar  = std::numeric_limits<float>::max();

    // For each X/Y/Z slab
    for(uint32_t ii=0; ii<3; ++ii)
    {
        float xxl = extent[2*ii];
        float xxh = extent[2*ii+1];
        float xxo = ray.origin_w[ii];
        float xxd = ray.direction[ii];

        // If ray parallel to planes
        if(fabs(xxd)<epsilon)
        {
            // If ray origin not between slab, no intersection for this slab
            if(xxo < xxl || xxo > xxh)
                return false;
        }
        else
        {
            // Compute the intersection distance of the planes
            float T1 = (xxl - xxo) / xxd;
            float T2 = (xxh - xxo) / xxd;

            if(T1 > T2)
                std::swap(T1, T2); // Make sure T1 is the intersection with the near plane
            if(T1 > Tnear)
                Tnear = T1; // Tnear will converge to the largest T1
            if(T2 < Tfar)
                Tfar = T2;  // Tfar will converge to the smallest T2
        }
    }

    // Box miss / box behind ray
    if(Tfar<Tnear || Tfar<0)
        return false;

    data.near = Tnear;
    data.far  = Tfar;
    return true;
}

bool ray_collides_OBB(const Ray& ray, const Model& model, RayCollisionData& data)
{
    // Transform ray to model space
    bool ret = ray_collides_extent(ray.to_model_space(const_cast<Model&>(model).get_model_matrix()), model.get_mesh().get_dimensions(), data);
    // Rescale hit data
    if(ret)
    {
        float scale = model.get_transformation().get_scale();
        data.near *= scale;
        data.far *= scale;
    }
    return ret;
}

// Collision traits full specs.
namespace traits
{
    // ------------------ WIP ------------------
    namespace detail
    {
        // Compute the maximum squared distance between an input real number and two real bounds
        // return 0 if input number is within bounds
        static float max_dist2_point_bounds(float pn, float bmin, float bmax)
        {
            if(pn < bmin)
            {
                float val = (bmin - pn);
                return val * val;
            }

            if(pn > bmax)
            {
                float val = (pn - bmax);
                return val * val;
            }

            return 0.f;
        }
    }

    template<>
    bool collision<BoundingRegion,BoundingRegion>::intersects(const BoundingRegion& va, const BoundingRegion& vb)
    {
        for(uint8_t ii=0; ii<3; ++ii)
            if(std::fabs(va.mid_point[ii] - vb.mid_point[ii]) > (va.half[ii] + vb.half[ii]))
                return false;

        return true;
    }
    template<>
    bool collision<BoundingRegion,math::vec3>::intersects(const BoundingRegion& va, const math::vec3& point)
    {
        for(uint8_t ii=0; ii<3; ++ii)
        {
            if(point[ii] <  va.extent[2*ii])   return false;
            if(point[ii] >= va.extent[2*ii+1]) return false;
        }

        return true;
    }
    template<>
    bool collision<Sphere,math::vec3>::intersects(const Sphere& sphere, const math::vec3& point)
    {
        float dist2 = (sphere.center-point).norm2();
        return (dist2 <= sphere.radius*sphere.radius);
    }
    template<>
    bool collision<Sphere,BoundingRegion>::intersects(const Sphere& sphere, const BoundingRegion& vb)
    {
        const math::vec3& center = sphere.center;

        // Squared distance from center to furthest point
        float sq = 0.0;
        sq += detail::max_dist2_point_bounds(center[0], vb.extent[0], vb.extent[1]);
        sq += detail::max_dist2_point_bounds(center[1], vb.extent[2], vb.extent[3]);
        sq += detail::max_dist2_point_bounds(center[2], vb.extent[4], vb.extent[5]);

        return (sq <= sphere.radius*sphere.radius);
    }

    template<>
    bool collision<BoundingRegion,BoundingRegion>::contains(const BoundingRegion& va, const BoundingRegion& vb)
    {
        for(uint8_t ii=0; ii<3; ++ii)
        {
            if(vb.extent[2*ii]   < va.extent[2*ii])   return false;
            if(vb.extent[2*ii+1] > va.extent[2*ii+1]) return false;
        }

        return true;
    }
    template<>
    bool collision<BoundingRegion,math::vec3>::contains(const BoundingRegion& va, const math::vec3& point)
    {
        for(uint8_t ii=0; ii<3; ++ii)
        {
            if(point[ii] <  va.extent[2*ii])   return false;
            if(point[ii] >= va.extent[2*ii+1]) return false;
        }

        return true;
    }

    template<>
    bool collision<FrustumBox,math::vec3>::intersects(const FrustumBox& fb, const math::vec3& point)
    {
        // * Point has to be above all frustum planes in order to be in frustum
        // For each frustum plane, check if point is above plane, bail early if not the case
        for(uint32_t ii=0; ii<6; ++ii)
            if(fb.dist_to_plane(ii, point)<0)
                return false;

        return true;
    }
    template<>
    bool collision<FrustumBox,Sphere>::intersects(const FrustumBox& fb, const Sphere& sphere)
    {
        float dist01 = fmin(fb.dist_to_plane(0, sphere.center), fb.dist_to_plane(1, sphere.center));
        float dist23 = fmin(fb.dist_to_plane(2, sphere.center), fb.dist_to_plane(3, sphere.center));
        float dist45 = fmin(fb.dist_to_plane(4, sphere.center), fb.dist_to_plane(5, sphere.center));
        return (fmin(fmin(dist01, dist23), dist45) + sphere.radius)>0;
    }
    // ------------------ WIP ------------------
}

}
