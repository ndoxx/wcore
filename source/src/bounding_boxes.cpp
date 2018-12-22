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

OBB::OBB(Model& parent):
parent_transform_(parent.get_transformation()),
extent_(parent.get_mesh().get_dimensions())
{
    // Initialize proper scale to parent's intrinsic scale
    proper_scale_.init_scale(vec3(extent_[1]-extent_[0],
                                  extent_[3]-extent_[2],
                                  extent_[5]-extent_[4]));

    // ~HACK translate vertically by default
    offset_.init_translation(vec3(0,extent_[3]*0.5f,0));
}

OBB::~OBB() = default;

void OBB::update()
{
    // Compute OBB transform from parent transform
    proper_transform_ = parent_transform_.get_model_matrix() * offset_ * proper_scale_;

    // Compute OBB vertices
    for(uint32_t ii=0; ii<8; ++ii)
        vertices_[ii] = proper_transform_ * CENTERED_CUBE_VERTICES[ii];
}


AABB::AABB(Model& parent):
parent_transform_(parent.get_transformation())
{
    // Initialize proper scale to parent's intrinsic scale
    const extent_t& pdim = parent.get_mesh().get_dimensions();
    proper_scale_.init_scale(vec3(pdim[1]-pdim[0],
                                  pdim[3]-pdim[2],
                                  pdim[5]-pdim[4]));

    offset_.init_identity();
}

AABB::~AABB() = default;

void AABB::update()
{
    // Compute OBB transform from parent transform
    proper_transform_ = parent_transform_.get_model_matrix() * offset_ * proper_scale_;

    // Compute OBB vertices and extent
    std::vector<vec3> OBB_vertices;
    for(uint32_t ii=0; ii<8; ++ii)
    {
        vec3 vertex(proper_transform_ * CUBE_VERTICES[ii]);
        OBB_vertices.push_back(vertex);
    }
    math::compute_extent(OBB_vertices, extent_);

    // Create AABB from OBB extent
    mat4 AABB_scale, AABB_pos;
    AABB_scale.init_scale(vec3(extent_[1]-extent_[0],
                               extent_[3]-extent_[2],
                               extent_[5]-extent_[4]));
    AABB_pos.init_translation(vec3((extent_[1]+extent_[0])*0.5f,
                                   (extent_[3]+extent_[2])*0.5f,
                                   (extent_[5]+extent_[4])*0.5f));

    // Compute AABB vertices
    proper_transform_ = AABB_pos * offset_ * AABB_scale;
    for(uint32_t ii=0; ii<8; ++ii)
        vertices_[ii] = proper_transform_ * CUBE_VERTICES[ii];
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
    const vec3& forward = camera.get_forward();

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

bool FrustumBox::collides_sphere(const math::vec3& center, float radius) const
{
   float dist01 = fmin(dist_to_plane(0, center), dist_to_plane(1, center));
   float dist23 = fmin(dist_to_plane(2, center), dist_to_plane(3, center));
   float dist45 = fmin(dist_to_plane(4, center), dist_to_plane(5, center));
   return (fmin(fmin(dist01, dist23), dist45) + radius)>0;
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

        /*std::cout << "ii: " << ii << " xxl: " << xxl
                  << " xxh: " << xxh << " xxo: " << xxo
                  << " xxd: " << xxd  << std::endl;*/

        // If ray parallel to planes
        if(fabs(xxd)<epsilon)
        {
            // If ray origin not between slab, no intersection for this slab
            if(xxo < xxl || xxo > xxh)
            {
                //std::cout << "// ray origin out of slab" << std::endl;
                return false;
            }
        }
        else
        {
            // Compute the intersection distance of the planes
            float T1 = (xxl - xxo) / xxd;
            float T2 = (xxh - xxo) / xxd;

            //std::cout << "T1: " << T1 << " T2: " << T2 << std::endl;

            if(T1 > T2)
                std::swap(T1, T2); // Make sure T1 is the intersection with the near plane
            if(T1 > Tnear)
                Tnear = T1; // Tnear will converge to the largest T1
            if(T2 < Tfar)
                Tfar = T2;  // Tfar will converge to the smallest T2

            //std::cout << "Tnear: " << Tnear << " Tfar: " << Tfar << std::endl;
        }
    }

    // Box miss / box behind ray
    if(Tfar<Tnear || Tfar<0)
    {
        //std::cout << "miss" << std::endl;
        return false;
    }

    //std::cout << "hit: Tnear:" << Tnear << " Tfar: " << Tfar << std::endl;
    data.near = Tnear;
    data.far  = Tfar;
    return true;
}

bool ray_collides_OBB(const Ray& ray, std::shared_ptr<Model> pmdl, RayCollisionData& data)
{
    // Transform ray to model space
    bool ret = ray_collides_extent(ray.to_model_space(pmdl->get_model_matrix()), pmdl->get_mesh().get_dimensions(), data);
    if(ret)
    {
        float scale = pmdl->get_transformation().get_scale();
        data.near *= scale;
        data.far *= scale;
    }
    return ret;
}

}
