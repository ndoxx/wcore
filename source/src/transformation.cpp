#include "transformation.h"

using namespace math;

Transformation::Transformation():
position_(0.0, 0.0, 0.0),
orientation_(0.0, 0.0, 0.0),
scale_(1.0),
scaled_(false)
{

}

Transformation::Transformation(const math::vec3& position, const math::quat& orientation, float scale):
position_(position),
orientation_(orientation),
scale_(scale),
scaled_((scale!=1.0f))
{

}

Transformation::Transformation(math::vec3&& position, math::quat&& orientation, float scale):
position_(std::move(position)),
orientation_(std::move(orientation)),
scale_(scale),
scaled_((scale!=1.0f))
{

}

math::mat4 Transformation::get_model_matrix()
{
    mat4 T, R;
    T.init_translation(position_);
    R = orientation_.get_rotation_matrix();
    // If no scaling (scale_==1.0f) just return product of translation and rotation matrices.
    if (!scaled_)
        return T*R;

    // Else, compute a uniform scale matrix and return total product of transformations.
    mat4 S;
    S.init_scale(scale_);
    return T*R*S;
}

math::mat4 Transformation::get_scale_rotation_matrix()
{
    mat4 R;
    R = orientation_.get_rotation_matrix();
    // If no scaling (scale_==1.0f) just return product of translation and rotation matrices.
    if (!scaled_)
        return R;

    // Else, compute a uniform scale matrix and return total product of transformations.
    mat4 S;
    S.init_scale(scale_);
    return R*S;
}

void Transformation::rotate(float phi, float theta, float psi)
{
    quat incr(phi, theta, psi);
    orientation_ *= incr;
}

void Transformation::rotate(const vec3& axis, float angle)
{
    quat incr(axis, angle);
    orientation_ *= incr;
}

void Transformation::rotate(vec3&& axis, float angle)
{
    quat incr(std::forward<vec3>(axis), angle);
    orientation_ *= incr;
}

void Transformation::rotate_around(const math::vec3& point, const math::vec3& axis, float angle)
{
    quat q1(vec4(position_-point, 0.0));
    quat q2(axis, angle);
    quat rot_dist(q2.get_conjugate()*(q1*q2));
    position_ = rot_dist.get_as_vec().xyz();
}

Transformation Transformation::operator*(const Transformation& other) const
{
    Transformation newtrans(position_    + other.position_,
                            orientation_ * other.orientation_,
                            scale_       * other.scale_);

    return newtrans;
}

Transformation& Transformation::operator*=(const Transformation& other)
{
    position_    += other.position_;
    orientation_ *= other.orientation_;
    scale_       *= other.scale_;
    scaled_      |= other.scaled_;
    return *this;
}
