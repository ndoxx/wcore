#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include "math3d.h"

namespace wcore
{

class Transformation
{
private:
    math::vec3 position_;
    math::quat orientation_;
    float scale_;
    bool scaled_;

public:
    Transformation();
    Transformation(const math::vec3& position, const math::quat& orientation, float scale=1.0f);
    Transformation(math::vec3&& position, math::quat&& orientation, float scale=1.0f);

    inline void set_position(const math::vec3& position) { position_ = position; }
    inline void set_position(math::vec3&& position) { std::swap(position_, position); }
    inline void set_orientation(const math::quat& orientation) { orientation_ = orientation; }
    inline void set_orientation(math::quat&& orientation) { std::swap(orientation_, orientation); }
    inline void set_scale(float scale) { scale_ = scale; scaled_ = (scale!=1.0f); }

    inline const math::vec3& get_position() const { return position_; }
    inline math::vec3& get_position_nc() { return position_; }
    inline const math::quat& get_orientation() const { return orientation_; }
    inline float get_scale() const { return scale_; }
    inline bool  is_scaled() const { return scaled_; }
    inline math::mat4 get_scale_matrix() const
    {
        math::mat4 S;
        S.init_scale(scale_);
        return S;
    }

    void rotate(float phi, float theta, float psi);
    void rotate(const math::vec3& axis, float angle);
    void rotate(math::vec3&& axis, float angle);

    //IMPLEMENT THIS
    void rotate_around(const math::vec3& point, const math::vec3& axis, float angle);
    inline void rotate_around(const math::vec3& point, float angle) { rotate_around(point, math::vec3(0.0, 1.0, 0.0), angle); }

    inline void translate(const math::vec3& increment) { position_ += increment; }
    inline void translate(float x, float y, float z)   { position_ += math::vec3(x,y,z); }
    inline void translate_x(float x)                   { position_ += math::vec3(x,0,0); }
    inline void translate_y(float y)                   { position_ += math::vec3(0,y,0); }
    inline void translate_z(float z)                   { position_ += math::vec3(0,0,z); }


    inline void reset_position()    { position_.zero(); }
    inline void reset_orientation() { orientation_.set_identity(); }
    inline void reset_scale()       { scale_  = 1.0f;
                                      scaled_ = false; }
    inline void set_identity()      { position_.zero();
                                      orientation_.set_identity();
                                      scale_  = 1.0f;
                                      scaled_ = false; }

    math::mat4 get_model_matrix();
    math::mat4 get_scale_rotation_matrix();
    math::mat4 get_rotation_translation_matrix();

    Transformation operator*(const Transformation& other) const;
    Transformation& operator*=(const Transformation& other);
};

}

#endif // TRANSFORMATION_H
