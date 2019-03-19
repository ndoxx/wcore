#include <algorithm>
#include <array>

#include "camera.h"
#include "algorithms.h"
#include "config.h"
#include "logger.h"

#ifndef __DISABLE_EDITOR__
    #include "imgui/imgui.h"
    #include "gui_utils.h"
#endif

namespace wcore
{

using namespace math;

float Camera::MAX_PITCH = 89.0f;

Camera::Camera(float scr_width, float scr_height):
pitch_(0.0f),
yaw_(0.0f),
dt_(0.0f),
proj_(),
position_(0.0f,0.0f,0.0f),
lookat_(0.0f,0.0f,0.0f),
update_frustum_(true),
is_ortho_(false)
{
    CONFIG.get("root.camera.near"_h, NEAR);
    CONFIG.get("root.camera.far"_h, FAR);
    CONFIG.get("root.camera.speed_slow"_h, SPEED_SLOW);
    CONFIG.get("root.camera.speed_fast"_h, SPEED_FAST);
    CONFIG.get("root.camera.speed_rot"_h, rot_speed_);
    speed_ = SPEED_SLOW;

    set_perspective(scr_width, scr_height, NEAR, FAR);

    CONFIG.get("root.input.mouse.sensitivity"_h, MOUSE_SENSITIVITY_X);
    MOUSE_SENSITIVITY_Y = MOUSE_SENSITIVITY_X * ((CONFIG.is("root.input.mouse.y_inverted"_h))?-1.0f:1.0f);
}

void Camera::set_perspective(float scr_width, float scr_height, float z_near, float z_far)
{
    NEAR = z_near;
    FAR = z_far;
    frustum_.init(-scr_width/(scr_height)*NEAR, scr_width/(scr_height)*NEAR, -NEAR, NEAR, NEAR, FAR);
    init_frustum(proj_, frustum_);
    compute_rays_perspective();
    is_ortho_ = false;
}

void Camera::set_orthographic(const std::array<float,6>& extent)
{
    frustum_.init(extent[0], extent[1], extent[2], extent[3], extent[4], extent[5]);
    init_ortho(proj_, frustum_);
    is_ortho_ = true;
}


void Camera::set_orthographic(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
    frustum_.init(xmin, xmax, ymin, ymax, zmin, zmax);
    init_ortho(proj_, frustum_);
    is_ortho_ = true;
}


void Camera::set_orthographic(float scr_width, float scr_height, float zoom)
{
    float w2 = 1.0f/zoom * scr_width/2.0f;
    float h2 = 1.0f/zoom * scr_height/2.0f;
    frustum_.init(-w2, w2, -h2, h2, NEAR, FAR);
    init_ortho(proj_, frustum_);
    is_ortho_ = true;
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
    is_ortho_ = false;
}

void Camera::update(float dt)
{
    // Update frame interval
    dt_ = dt;

    // * Update frustum bounding box
    if(update_frustum_)
        frusBox_.update(*this);
}

void Camera::freefly_view()
{
    // * Update matrices
    // First, compute camera model matrix
    mat4 R,T;
    init_rotation_tait_bryan(R, 0.0f, TORADIANS(yaw_), TORADIANS(pitch_));
    init_translation(T, position_);
    model_ = T*R;

    // Invert it to obtain the view matrix
    math::inverse_affine(model_, view_);

    // * Extract proper axes
    right_   = vec3(model_.col(0));
    up_      = vec3(model_.col(1));
    forward_ = vec3(model_.col(2));
}

void Camera::look_at_view()
{
    // Initialize view matrix
    math::init_look_at(view_, position_, lookat_, vec3(0,1,0));
    // Invert it to obtain the model matrix
    math::inverse_affine(view_, model_);

    // Get each axis
    right_   = vec3(model_.col(0));
    up_      = vec3(model_.col(1));
    forward_ = vec3(model_.col(2));

    // Update frustum bounding box
    if(update_frustum_)
        frusBox_.update(*this);
}

float Camera::get_frustum_diagonal() const
{
    return (frusBox_.RBN()-frusBox_.LTF()).norm();
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
void Camera::get_truncated_frustum_corners(float ymin, std::array<vec3, 8>& destination) const
{
    const std::array<vec3, 8>& corners = frusBox_.get_corners();

    // check right bottom far corner y, if y>ymin bail early with unaltered frustum
    if(corners[1].y()>ymin)
    {
        std::copy(corners.begin(), corners.end(), destination.begin());
        return;
    }

    // Find parameter of bottom right line such that y=ymin
    float y_rbn = corners[0].y();
    float y_rbf = corners[1].y();
    float ratio = (ymin - y_rbn) / (y_rbf - y_rbn);

    // THIS IS THE RIGHT WAY TO DO IT if we have parallel split frusta
    /*vec3 cnear(math::lerp(corners[0], corners[7], 0.5f));
    vec3 cfar (math::lerp(corners[1], corners[6], 0.5f));

    if(cfar.y()>ymin)
    {
        std::copy(corners.begin(), corners.end(), destination.begin());
        return;
    }

    // Find parameter of view line such that y=ymin
    float y_n = cnear.y();
    float y_f = cfar.y();
    float ratio = (ymin - y_n) / (y_f - y_n);*/

    // Push corrected vertices
    destination[0] = (corners[0]);
    destination[1] = (math::lerp(corners[0], corners[1], ratio));
    destination[2] = (math::lerp(corners[3], corners[2], ratio));
    destination[3] = (corners[3]);
    destination[4] = (corners[4]);
    destination[5] = (math::lerp(corners[4], corners[5], ratio));
    destination[6] = (math::lerp(corners[7], corners[6], ratio));
    destination[7] = (corners[7]);
}

void Camera::set_orthographic_tight_fit(const Camera& other,
                                        const math::vec3& view_dir,
                                        float texel_size_x,
                                        float texel_size_y)
{
    // Get other camera frustum corners in world space
    //const std::array<vec3, 8>& corners_world = other.get_frustum_corners();
    std::array<vec3, 8> corners_world;
    other.get_truncated_frustum_corners(-10, corners_world);

    // Get center of view frustum
    //vec3 frus_center(math::lerp(corners_world[0], corners_world[6], 0.5f));

    // Move camera along view direction
    set_position(100.0f*view_dir/*+other.position_*/);

    // Look at target position
    set_look_at(vec3(0)/*+other.position_*/);
    look_at_view();

    // Transform corners from world to view space
    static std::array<vec3, 8> corners_lightspace;
    std::transform(corners_world.begin(), corners_world.end(), corners_lightspace.begin(),
                   [&](const math::vec3& pos_w) -> vec3 { return view_*pos_w; });

    // * Setup camera
    // Compute extent of vertices in view space
    static std::array<float,6> extent;
    math::compute_extent(corners_lightspace, extent);

    /*BANG();
    for(int ii=0; ii<8; ++ii)
        std::cout << corners_world[ii] << " ";
    std::cout << std::endl;*/

    /*for(int ii=0; ii<6; ++ii)
        std::cout << extent[ii] << " ";
    std::cout << std::endl;*/

    // * Don't allow frustum to shrink below the size of other cam long diagonal
    [[maybe_unused]] float full_diagonal = other.get_frustum_diagonal();
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
    /*float zoom = 1.0f/((!(diagonal<full_diagonal))?1.5f:1.2f);
    for(uint8_t ii=0; ii<4; ++ii)
    {
        extent[ii] *= zoom;
    }*/

    if(texel_size_x && texel_size_y)
    {
        // * Round to the nearest texel (limits shadow flickering)
        vec2 units_per_texel = 2.0f * vec2((extent[1]-extent[0])*texel_size_x,
                                           (extent[3]-extent[2])*texel_size_y);
        extent[0] = floor(extent[0]/units_per_texel.x()) * units_per_texel.x();
        extent[1] = floor(extent[1]/units_per_texel.x()) * units_per_texel.x();
        extent[2] = floor(extent[2]/units_per_texel.y()) * units_per_texel.y();
        extent[3] = floor(extent[3]/units_per_texel.y()) * units_per_texel.y();

        // fixed-Z, because tight fit z-bounds would cause occluder clipping
        // to be [REPLACE]d with proper scene AABB query
        extent[4] = -10.0f;
        extent[5] = 200.0f;
        //extent[5] = 100.0f + (100.0f-other.position_.y());
    }

    // * Set orthographic perspective
    set_orthographic(extent);
}

#ifndef __DISABLE_EDITOR__
void Camera::generate_gui_element()
{
    ImGui::Text("Position: x=%f, y=%f, z=%f", position_.x(), position_.y(), position_.z());
    ImGui::Text("Pitch: %f, Yaw: %f", pitch_, yaw_);
    ImGui::Text("Velocity: %f, Rot speed: %f", speed_, rot_speed_);
}
#endif

}
