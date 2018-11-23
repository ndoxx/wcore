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
private:
    float           pitch_;
    float           yaw_;
    float           dt_;
    float           speed_;
    float           rot_speed_;

    math::Frustum   frustum_;
    math::mat4      proj_;
    math::mat4      view_;
    math::mat4      model_;
    math::vec3      position_;
    math::vec3      right_;
    math::vec3      up_;
    math::vec3      forward_;

    std::array<math::vec2,4> rays_;

    FrustumBox      frusBox_;
    bool            update_frustum_;

    static float MAX_PITCH;
    static float NEAR;
    static float FAR;
    static float MOUSE_SENSITIVITY_X;
    static float MOUSE_SENSITIVITY_Y;

public:
    static const float SPEED_SLOW;
    static const float SPEED_FAST;


    Camera() = delete;
    Camera(float scr_width, float scr_height);

    void set_perspective(float scr_width, float scr_height, float z_far=100.0f);
    inline void set_perspective() { init_frustum(proj_, frustum_); }

    void set_orthographic(float extent[6]);
    void set_orthographic(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
    void set_orthographic(float scr_width, float scr_height, float zoom=1.0f);
    inline void set_orthographic() { init_ortho(proj_, frustum_); }
    void set_hybrid_perspective(float scr_width, float scr_height, float alpha, float z_far=100.0f);

    inline const math::Frustum& get_frustum() const { return frustum_; }

    inline void update_orientation(float d_yaw, float d_pitch);
    inline void update_position(const math::vec3& d_pos);

    inline void set_orientation(float yaw, float pitch);
    inline void set_position(const math::vec3& newpos);
    inline const math::vec3& get_position() const { return position_; }
    inline float get_yaw() const { return yaw_; }
    inline float get_pitch() const { return pitch_; }

    inline const math::mat4& get_view_matrix() const;
    inline const math::mat4& get_model_matrix() const;
    inline const math::mat4& get_projection_matrix() const;
    inline const std::array<math::vec2,4>& get_rays() const;

    inline const math::vec3& get_right() const;
    inline const math::vec3& get_up() const;
    inline const math::vec3& get_forward() const;

    inline void set_speed(float value) { speed_ = value; }
    inline float get_speed() const { return speed_; }

    inline void move_forward();
    inline void move_backward();
    inline void ascend();
    inline void descend();
    inline void strafe_right();
    inline void strafe_left();

    inline bool frustum_collides(const AABB& aabb) { return frusBox_.collides(aabb); }
    inline bool frustum_collides(const OBB& obb)   { return frusBox_.collides(obb); }
    inline bool frustum_collides_sphere(const math::vec3& center, float radius) const;
    inline void enable_frustum_update()  { update_frustum_ = true; }
    inline void disable_frustum_update() { update_frustum_ = false; }
    inline const std::array<math::vec3, 8>& get_frustum_corners() const { return frusBox_.get_corners(); }
    inline math::vec3 get_frustum_split_center(uint32_t splitIndex) const;

    float get_frustum_diagonal() const;
    void get_truncated_frustum_corners(float ymin, std::vector<math::vec3>& destination) const;
    void set_orthographic_tight_fit(const Camera& other,
                                    const math::vec3& view_dir,
                                    float texel_size_x = 0.0f,
                                    float texel_size_y = 0.0f);

    void look_at(const math::vec3& posLookAt);
    void update(float dt);

private:
    void compute_rays_perspective();
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

inline void Camera::set_orientation(float yaw, float pitch)
{
    yaw_ = yaw;
    pitch_ = std::min(MAX_PITCH, pitch);
}

inline void Camera::move_forward()
{
    const math::vec3& f = get_forward();
    update_position(math::vec3(f.x(), 0.0f, f.z()).normalized()*dt_);
}

inline void Camera::move_backward()
{
    const math::vec3& f = get_forward();
    update_position(math::vec3(f.x(), 0.0f, f.z()).normalized()*(-dt_));
}

inline void Camera::ascend()
{
    update_position(math::vec3(0.0, 0.5f*speed_*dt_, 0.0));
}

inline void Camera::descend()
{
    update_position(math::vec3(0.0, 0.5f*speed_*(-dt_), 0.0));
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

inline void Camera::update_position(const math::vec3& d_pos)
{
    position_ += speed_*d_pos;
}

inline bool Camera::frustum_collides_sphere(const math::vec3& center, float radius) const
{
    return frusBox_.collides_sphere(center, radius);
}

inline const math::mat4& Camera::get_view_matrix() const
{
    return view_;
}

inline const math::mat4& Camera::get_model_matrix() const
{
    return model_;
}

inline const math::mat4& Camera::get_projection_matrix() const
{
    return proj_;
}

inline const std::array<math::vec2,4>& Camera::get_rays() const
{
    return rays_;
}

inline math::vec3 Camera::get_frustum_split_center(uint32_t splitIndex) const
{
    return frusBox_.split_center(splitIndex);
}

}

#endif // CAMERA_H
