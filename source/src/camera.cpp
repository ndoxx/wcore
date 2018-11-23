#include <algorithm>
#include <array>

#include "camera.h"
#include "algorithms.h"
#include "config.h"
#include "logger.h"

namespace wcore
{

using namespace math;

float Camera::MAX_PITCH = 89.0f;
float Camera::NEAR = 0.1f;
float Camera::FAR = 100.0f;
float Camera::MOUSE_SENSITIVITY_X = 1.0f;
float Camera::MOUSE_SENSITIVITY_Y = 1.0f;
const float Camera::SPEED_SLOW = 4.0f;
const float Camera::SPEED_FAST = 20.0f;

Camera::Camera(float scr_width, float scr_height):
pitch_(0.0f),
yaw_(0.0f),
dt_(0.0f),
speed_(4.0f),
rot_speed_(30.0f),
frustum_({-scr_width/(scr_height)*NEAR, scr_width/(scr_height)*NEAR, -NEAR, NEAR, NEAR, FAR}),
proj_(),
position_(0.0f,0.0f,0.0f),
update_frustum_(true)
{
    init_frustum(proj_, frustum_);
    compute_rays_perspective();

    CONFIG.get(H_("root.input.mouse.sensitivity"), MOUSE_SENSITIVITY_X);
    MOUSE_SENSITIVITY_Y = MOUSE_SENSITIVITY_X * ((CONFIG.is(H_("root.input.mouse.y_inverted")))?-1.0f:1.0f);
}

void Camera::set_perspective(float scr_width, float scr_height, float z_far)
{
    frustum_.l = -scr_width/(scr_height)*NEAR;
    frustum_.r = scr_width/(scr_height)*NEAR;
    frustum_.f = z_far;
    init_frustum(proj_, frustum_);
    compute_rays_perspective();
}

void Camera::set_orthographic(float extent[6])
{
    frustum_.init(extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);
    init_ortho(proj_, frustum_);
}


void Camera::set_orthographic(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
    frustum_.init(xmin, xmax, ymin, ymax, zmin, zmax);
    init_ortho(proj_, frustum_);
}


void Camera::set_orthographic(float scr_width, float scr_height, float zoom)
{
    float w2 = 1.0f/zoom * scr_width/2.0f;
    float h2 = 1.0f/zoom * scr_height/2.0f;
    frustum_.init(-w2, w2, -h2, h2, NEAR, FAR);
    init_ortho(proj_, frustum_);
}

// If needed refactor so as to cache two matrices for persp instead of computing them each time
void Camera::set_hybrid_perspective(float scr_width, float scr_height, float alpha, float z_far)
{
    frustum_.l = -scr_width/(scr_height);
    frustum_.r = scr_width/(scr_height);
    frustum_.f = z_far;
    mat4 Portho, Ppersp;
    init_ortho(Portho, frustum_);
    init_frustum(Ppersp, frustum_);

    // By convention alpha=0 is full ortho, alpha=1 is full perspective
    proj_ = lerp(Portho, Ppersp, alpha);
}

void Camera::update(float dt)
{
    // Update frame interval
    dt_ = dt;

    // Update matrices
    mat4 R;
    init_rotation_euler(R, 0.0f, TORADIANS(yaw_), TORADIANS(pitch_));
    mat4 T;
    init_translation(T, -position_);
    view_ = R*T;

    R.transpose();
    init_translation(T, position_);
    model_ = T*R;

    right_ = vec3(view_.row(0)).normalized();
    up_ = vec3(view_.row(1)).normalized();
    forward_ = -vec3(view_.row(2)).normalized(); // WTF -

    // Update frustum bounding box
    if(update_frustum_)
        frusBox_.update(*this);
}

float Camera::get_frustum_diagonal() const
{
    return (frusBox_.RBN()-frusBox_.LTF()).norm();
}


void Camera::look_at(const math::vec3& posLookAt)
{
    // Initialize view matrix
    math::init_look_at(view_, position_, posLookAt, vec3(0,1,0));

    // Get each axis
    right_ = vec3(view_.row(0));
    up_ = vec3(view_.row(1));
    forward_ = -vec3(view_.row(2)); // WTF -

    // Update frustum bounding box
    if(update_frustum_)
        frusBox_.update(*this);
}

void Camera::compute_rays_perspective()
{
    // Homogeneous positions of screen corners
    // Must be the same order as vertices in factory::make_quad_3P2U()
    std::vector<vec4> corners({vec4(-1,-1,0,1),
                               vec4(1,-1,0,1),
                               vec4(-1,1,0,1),
                               vec4(1,1,0,1)});
    // Compute inverse projection to "unproject" corners
    mat4 invProjection;
    math::inverse(proj_, invProjection);

    // Unproject rays from NDC to view space
    for(uint32_t ii=0; ii<4; ++ii)
    {
        vec4 ray(invProjection*corners[ii]);
        ray /= ray.w();
        ray /= ray.z(); // z-normalization
        rays_[ii] = vec2(ray);
    }
}

// optimized shadow frustum fit for top viewed scenes
void Camera::get_truncated_frustum_corners(float ymin, std::vector<math::vec3>& destination) const
{
    const std::array<vec3, 8>& corners = frusBox_.get_corners();

    // check right top far corner y, if y>ymin bail early with unaltered frustum
    if(corners[5].y()>ymin)
    {
        destination.resize(corners.size());
        std::copy(corners.begin(), corners.end(), destination.begin());
        return;
    }

    // Right bottom far corner will have the minimal y value
    float minimum = corners[1].y();
    // and right bottom near corner norm is the visible portion of edge
    float dist_visible = sqrt(corners[0].x()*corners[0].x() + corners[0].y()*corners[0].y());
    // Compute ratio of visible portion of frustum
    float ratio  = dist_visible/(dist_visible-minimum);

    // Push corrected vertices
    destination.push_back(corners[0]);
    destination.push_back(math::lerp(corners[0], corners[1], ratio));
    destination.push_back(math::lerp(corners[3], corners[2], ratio));
    destination.push_back(corners[3]);
    destination.push_back(corners[4]);
    destination.push_back(math::lerp(corners[4], corners[5], ratio));
    destination.push_back(math::lerp(corners[7], corners[6], ratio));
    destination.push_back(corners[7]);
}

void Camera::set_orthographic_tight_fit(const Camera& other,
                                        const math::vec3& view_dir,
                                        float texel_size_x,
                                        float texel_size_y)
{
    // Get other camera frustum corners in world space
    const std::array<vec3, 8>& corners_world = other.get_frustum_corners();
    //std::vector<math::vec3> corners_world;
    //other.get_truncated_frustum_corners(10, corners_world);

    // Get center of view frustum
    /*vec3 frus_center(math::lerp(corners_world[0], corners_world[6], 0.5f));*/

    // Move camera to position with an offset (view direction)
    set_position(view_dir+other.position_);
    // Look at target position
    look_at(other.position_);

    // Transform corners from world to view space
    static std::array<vec3, 8> corners_lightspace;
    std::transform(corners_world.begin(), corners_world.end(), corners_lightspace.begin(),
                   [&](const math::vec3& pos_w) -> vec3 { return view_*pos_w; });

    // * Setup camera
    // Compute extent of vertices in view space
    static float extent[6];
    math::compute_extent(corners_lightspace, extent);

    // * Don't allow frustum to shrink below the size of other cam long diagonal
    float full_diagonal = other.get_frustum_diagonal();
    float diagonal = (corners_world[0]-corners_world[6]).norm();
    float x_mid = 0.5f*(extent[0]+extent[1]);
    float y_mid = 0.5f*(extent[2]+extent[3]);
    float x_size = extent[1]-extent[0];
    float y_size = extent[3]-extent[2];
    if(x_size<diagonal)
    {
        extent[0] = x_mid - 0.5f*diagonal;
        extent[1] = x_mid + 0.5f*diagonal;
    }
    if(y_size<diagonal)
    {
        extent[2] = y_mid - 0.5f*diagonal;
        extent[3] = y_mid + 0.5f*diagonal;
    }

    // [TMP] Zoom. This improves definition but can cause occluder clipping within the view frustum
    float zoom = 1.0f/((!(diagonal<full_diagonal))?2.5f:1.2f);
    for(uint8_t ii=0; ii<4; ++ii)
    {
        extent[ii] *= zoom;
    }

    if(texel_size_x && texel_size_y)
    {
        // * Round to the nearest texel (avoids shadow flickering)
        vec2 units_per_texel = 2.0f * vec2((extent[1]-extent[0])*texel_size_x,
                                           (extent[3]-extent[2])*texel_size_y);
        extent[0] = floor(extent[0]/units_per_texel.x()) * units_per_texel.x();
        extent[1] = floor(extent[1]/units_per_texel.x()) * units_per_texel.x();
        extent[2] = floor(extent[2]/units_per_texel.y()) * units_per_texel.y();
        extent[3] = floor(extent[3]/units_per_texel.y()) * units_per_texel.y();

        // fixed-Z, because tight fit z-bounds would cause occluder clipping
        // to be [REPLACE]d with proper scene AABB query
        //extent[4] = -10.0f;
        //extent[5] = 150.0f;

        extent[5] = other.position_.y()+20;
    }

    // * Set orthographic perspective
    set_orthographic(extent);
}

}
