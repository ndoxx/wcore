#ifndef CAMERA_H
#define CAMERA_H

#include <array>

#include "math3d.h"
#include "bounding_boxes.h"

namespace wcore
{

class AABB;
class Camera
{
public:

    enum ViewPolicy
    {
        ANGULAR,
        DIRECTIONAL
    };

    Camera() = delete;
    Camera(float scr_width, float scr_height);

    inline void set_view_policy(ViewPolicy policy) { view_policy_ = policy; }

    inline float get_near() { return NEAR; }
    inline float get_far()  { return FAR; }

    void set_perspective(float scr_width, float scr_height, float z_near=0.1f, float z_far=100.0f);
    inline void set_perspective() { init_frustum(proj_, frustum_); }

    void set_orthographic(const std::array<float,6>& extent);
    void set_orthographic(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
    void set_orthographic(float scr_width, float scr_height, float zoom=1.0f);
    inline void set_orthographic() { init_ortho(proj_, frustum_); }
    void set_hybrid_perspective(float scr_width, float scr_height, float alpha, float z_far=100.0f);

    inline bool is_orthographic() { return is_ortho_; }

    inline const math::Frustum& get_frustum() const { return frustum_; }

    inline void update_orientation(float d_yaw, float d_pitch);
    inline void update_position(const math::vec3& d_pos);

    inline void set_orientation(float yaw, float pitch);
    inline void set_position(const math::vec3& newpos);
    inline const math::vec3& get_position() const { return position_; }
    inline float get_yaw() const   { return yaw_; }
    inline float get_pitch() const { return pitch_; }

    inline const math::mat4& get_view_matrix() const        { return view_; }
    inline const math::mat4& get_projection_matrix() const  { return proj_; }
    inline math::mat4 get_view_projection_matrix() const    { return proj_*view_; }
    inline const std::array<math::vec2,4>& get_rays() const { return rays_; }

    inline const math::vec3& get_right() const;
    inline const math::vec3& get_up() const;
    inline const math::vec3& get_forward() const;

    inline void set_speed(float value) { speed_ = value; }
    inline void set_speed_slow()       { speed_ = SPEED_SLOW; }
    inline void set_speed_fast()       { speed_ = SPEED_FAST; }
    inline float get_speed() const     { return speed_; }

    inline void move_forward();
    inline void move_backward();
    inline void ascend();
    inline void descend();
    inline void strafe_right();
    inline void strafe_left();

    inline bool frustum_collides(const AABB& aabb) { return traits::collision<FrustumBox,AABB>::intersects(frusBox_, aabb); }
    inline bool frustum_collides(const OBB& obb)   { return traits::collision<FrustumBox,OBB>::intersects(frusBox_, obb); }
    inline bool frustum_collides_sphere(const math::vec3& center, float radius) const;
    inline void enable_frustum_update()  { update_frustum_ = true; }
    inline void disable_frustum_update() { update_frustum_ = false; }
    inline const FrustumBox& get_frustum_box() const { return frusBox_; }
    inline const std::array<math::vec3, 8>& get_frustum_corners() const { return frusBox_.get_corners(); }
    inline math::vec3 get_frustum_split_center(uint32_t splitIndex) const;

    float get_frustum_diagonal() const;
    void get_truncated_frustum_corners(float ymin, std::array<math::vec3, 8>& destination) const;
    void set_orthographic_tight_fit(const Camera& other,
                                    float texel_size_x = 0.0f,
                                    float texel_size_y = 0.0f);

    inline void set_look_at(const math::vec3& value) { lookat_ = value; }

    void update(float dt);

#ifndef __DISABLE_EDITOR__
    void generate_gui_element();
#endif

private:
    void compute_rays_perspective();

private:
private:
    float           pitch_;
    float           yaw_;
    float           dt_;
    float           speed_;
    float           rot_speed_;
    ViewPolicy      view_policy_;

    math::Frustum   frustum_;
    math::mat4      proj_;
    math::mat4      view_;
    math::vec3      position_;
    math::vec3      lookat_;
    math::vec3      right_;
    math::vec3      up_;
    math::vec3      forward_;

    std::array<math::vec2,4> rays_;

    FrustumBox      frusBox_;
    bool            update_frustum_;
    bool            is_ortho_;

    float NEAR;
    float FAR;
    float MOUSE_SENSITIVITY_X;
    float MOUSE_SENSITIVITY_Y;
    float SPEED_SLOW;
    float SPEED_FAST;

    static float MAX_PITCH;
};

inline const math::vec3& Camera::get_right() const
{
    return right_;
}

inline const math::vec3& Camera::get_up() const
{
    return up_;
}

inline const math::vec3& Camera::get_forward() const
{
    return forward_;
}

inline void Camera::set_position(const math::vec3& newpos)
{
    position_ = newpos;
}

inline void Camera::move_forward()
{
    const math::vec3& f = get_forward();
    update_position(math::vec3(f.x(), 0.0f, f.z()).normalized()*(-dt_));
}

inline void Camera::move_backward()
{
    const math::vec3& f = get_forward();
    update_position(math::vec3(f.x(), 0.0f, f.z()).normalized()*dt_);
}

inline void Camera::ascend()
{
    update_position(math::vec3(0.0, 0.25f*speed_*dt_, 0.0));
}

inline void Camera::descend()
{
    update_position(math::vec3(0.0, 0.25f*speed_*(-dt_), 0.0));
}

inline void Camera::strafe_right()
{
    update_position(get_right()*dt_);
}

inline void Camera::strafe_left()
{
    update_position(get_right()*(-dt_));
}

inline void Camera::update_orientation(float d_yaw, float d_pitch)
{
    yaw_ += rot_speed_*MOUSE_SENSITIVITY_X*d_yaw;
    pitch_ += rot_speed_*MOUSE_SENSITIVITY_Y*d_pitch;
    if(pitch_>MAX_PITCH || pitch_<-MAX_PITCH)
        pitch_ -= rot_speed_*MOUSE_SENSITIVITY_Y*d_pitch;
    if(yaw_>=360.0f)
        yaw_ -= 360.0f;
    if(yaw_<0.0f)
        yaw_ += 360.0f;
}

inline void Camera::set_orientation(float yaw, float pitch)
{
    pitch_ = (pitch<=MAX_PITCH)?pitch:MAX_PITCH;
    yaw_   = yaw;
    if(yaw_>=360.0f)
        yaw_ -= 360.0f;
    if(yaw_<0.0f)
        yaw_ += 360.0f;
}

inline void Camera::update_position(const math::vec3& d_pos)
{
    position_ += speed_*d_pos;
}

inline bool Camera::frustum_collides_sphere(const math::vec3& center, float radius) const
{
    return traits::collision<FrustumBox,Sphere>::intersects(frusBox_, Sphere(center, radius));
}

inline math::vec3 Camera::get_frustum_split_center(uint32_t splitIndex) const
{
    return frusBox_.split_center(splitIndex);
}

}

#endif // CAMERA_H
