#include <cmath>
#include <iostream>
#include "quaternion.h"
#include "math3d.h"

namespace wcore
{
namespace math
{

const float Quaternion::NORM_TOL = 1e-5;

Quaternion::Quaternion():
value_(0.0, 0.0, 0.0, 1.0){}

Quaternion::Quaternion(float v):
value_(v, v, v, 1.0){}

Quaternion::Quaternion(float x, float y, float z, float w):
value_(x, y, z, w){}

Quaternion::Quaternion(float phi, float theta, float psi)
{
    this->init_tait_bryan(phi, theta, psi);
}

Quaternion::Quaternion(const vec3& axis, float angle)
{
    this->init_axis_angle(axis, angle);
}

Quaternion::Quaternion(const vec4& components):
value_(components){}

Quaternion::Quaternion(const vec3& components):
value_(components){}

/*Quaternion::Quaternion(const Quaternion& other):
value_(other.value_){}

Quaternion::Quaternion(Quaternion&& other) noexcept:
value_(std::move(other.value_)){}*/

Quaternion::~Quaternion(){}

void Quaternion::init_tait_bryan(float phi, float theta, float psi)
{
    phi   = TORADIANS(phi/2.0);
    theta = TORADIANS(theta/2.0);
    psi   = TORADIANS(psi/2.0);

    //ZYX
    value_[0] = cos(phi)*cos(theta)*sin(psi) - sin(phi)*sin(theta)*cos(psi);
    value_[1] = cos(phi)*sin(theta)*cos(psi) + sin(phi)*cos(theta)*sin(psi);
    value_[2] = sin(phi)*cos(theta)*cos(psi) - cos(phi)*sin(theta)*sin(psi);
    value_[3] = cos(phi)*cos(theta)*cos(psi) + sin(phi)*sin(theta)*sin(psi);
}

void Quaternion::init_axis_angle(const vec3& axis, float angle)
{
    vec3 ax(axis.normalized());
    angle = TORADIANS(angle/2.0);
    float s = sin(angle);
    value_[0] = ax.x()*s;
    value_[1] = ax.y()*s;
    value_[2] = ax.z()*s;
    value_[3] = cos(angle);
}

void Quaternion::init_lookat(const vec3& sourcePoint, const vec3& destPoint)
{
    //vec3 forwardVector = (destPoint - sourcePoint).normalized();

    // TODO
}

void Quaternion::normalize(float tolerance)
{
    if(std::abs(value_.norm2()-1.0f) > tolerance)
        value_.normalize();
}

void Quaternion::conjugate()
{
    value_[0] = -value_[0];
    value_[1] = -value_[1];
    value_[2] = -value_[2];
}

float Quaternion::dot_vector(const Quaternion& other)
{
    return value_[0]*other.value_[0] + value_[1]*other.value_[1]
         + value_[2]*other.value_[2] + value_[3]*other.value_[3];
}


Quaternion Quaternion::get_conjugate() const
{
    return Quaternion(-value_[0], -value_[1], -value_[2], value_[3]);
}

Quaternion Quaternion::get_inverse() const
{
    Quaternion ret(this->get_conjugate());
    float n2 = value_.norm2();
    for(unsigned ii = 0; ii < 4; ++ii)
        ret.value_[ii] /= n2;
    return ret;
}

vec3 Quaternion::rotate(const vec3& vector) const
{
    //return get_rotation_matrix()*vector;
    return ((*this)*Quaternion(vector)*get_conjugate()).get_as_vec().xyz();
}

vec3 Quaternion::rotate_inverse(const vec3& vector) const
{
    // return get_conjugate().get_rotation_matrix()*vector;
    return (get_conjugate()*Quaternion(vector)*(*this)).get_as_vec().xyz();
}

vec3 Quaternion::get_euler_angles(bool degrees)
{
    normalize();
    float phi   = atan2(2.0*(value_[3]*value_[0] + value_[1]*value_[2]),
                        1.0 - 2.0*(value_[0]*value_[0] + value_[1]*value_[1]));
    float theta = asin (2.0*(value_[3]*value_[1] - value_[2]*value_[0]));
    float psi   = atan2(2.0*(value_[3]*value_[2] + value_[0]*value_[1]),
                        1.0 - 2.0*(value_[1]*value_[1] + value_[2]*value_[2]));

    if(degrees)
        return vec3(TODEGREES(phi), TODEGREES(theta), TODEGREES(psi));
    return vec3(phi, theta, psi);
}

vec4 Quaternion::get_axis_angle(bool degrees)
{
    normalize();
    float angle = 2.0f*acos(value_.w());
    if(degrees)
        return vec4(value_.xyz().normalize(), TODEGREES(angle));
    return vec4(value_.xyz().normalize(), angle);
}

mat4 Quaternion::get_rotation_matrix() const
{
    const_cast<vec4*>(&value_)->normalize();

    float r11 = 1.0f - 2.0f * (value_[1]*value_[1] + value_[2]*value_[2]);
    float r12 = 2.0f * (value_[0]*value_[1] - value_[2]*value_[3]);
    float r13 = 2.0f * (value_[0]*value_[2] + value_[1]*value_[3]);
    float r21 = 2.0f * (value_[0]*value_[1] + value_[2]*value_[3]);
    float r22 = 1.0f - 2.0f * (value_[0]*value_[0] + value_[2]*value_[2]);
    float r23 = 2.0f * (value_[1]*value_[2] - value_[0]*value_[3]);
    float r31 = 2.0f * (value_[0]*value_[2] - value_[1]*value_[3]);
    float r32 = 2.0f * (value_[1]*value_[2] + value_[0]*value_[3]);
    float r33 = 1.0f - 2.0f * (value_[0]*value_[0] + value_[1]*value_[1]);

    return mat4(r11, r12, r13, 0.0,
                r21, r22, r23, 0.0,
                r31, r32, r33, 0.0,
                0.0, 0.0, 0.0, 1.0);
}

/*Quaternion& Quaternion::operator=(const Quaternion& rhs)
{
    value_ = rhs.value_;
    return *this;
}*/

Quaternion& Quaternion::operator*=(const Quaternion& rhs)
{
    value_ = (rhs * *this).value_;
    normalize();
    return *this;
}

Quaternion& Quaternion::operator+=(const Quaternion& rhs)
{
    value_ = (rhs + *this).value_;
    //normalize();
    return *this;
}

Quaternion Quaternion::operator-() const
{
    Quaternion result(-value_[0],-value_[1],-value_[2],-value_[3]);
    return result;
}

Quaternion operator+(const Quaternion& lhs, const Quaternion& rhs)
{
    return Quaternion(lhs.value_ + rhs.value_);
}

Quaternion operator-(const Quaternion& lhs, const Quaternion& rhs)
{
    return Quaternion(lhs.value_ - rhs.value_);
}

Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs)
{
    float w = lhs.value_[3]*rhs.value_[3] - lhs.value_[0]*rhs.value_[0] - lhs.value_[1]*rhs.value_[1] - lhs.value_[2]*rhs.value_[2];
    float x = lhs.value_[3]*rhs.value_[0] + lhs.value_[0]*rhs.value_[3] + lhs.value_[1]*rhs.value_[2] - lhs.value_[2]*rhs.value_[1];
    float y = lhs.value_[3]*rhs.value_[1] - lhs.value_[0]*rhs.value_[2] + lhs.value_[1]*rhs.value_[3] + lhs.value_[2]*rhs.value_[0];
    float z = lhs.value_[3]*rhs.value_[2] + lhs.value_[0]*rhs.value_[1] - lhs.value_[1]*rhs.value_[0] + lhs.value_[2]*rhs.value_[3];

    return Quaternion(x, y, z, w);
}

Quaternion operator/(const Quaternion& lhs, const Quaternion& rhs)
{
    float n2 = rhs.value_.norm2();
    float w  = rhs.value_[3]*lhs.value_[3] + rhs.value_[0]*lhs.value_[0] + rhs.value_[1]*lhs.value_[1] + rhs.value_[2]*lhs.value_[2];
    float x  = rhs.value_[3]*lhs.value_[0] - rhs.value_[0]*lhs.value_[3] - rhs.value_[1]*lhs.value_[2] + rhs.value_[2]*lhs.value_[1];
    float y  = rhs.value_[3]*lhs.value_[1] + rhs.value_[0]*lhs.value_[2] - rhs.value_[1]*lhs.value_[3] - rhs.value_[2]*lhs.value_[0];
    float z  = rhs.value_[3]*lhs.value_[2] - rhs.value_[0]*lhs.value_[1] + rhs.value_[1]*lhs.value_[0] - rhs.value_[2]*lhs.value_[3];

    return Quaternion(x/n2, y/n2, z/n2, w/n2);
}

Quaternion operator*(const Quaternion& lhs, float rhs)
{
    return Quaternion(lhs.value_ * rhs);
}

Quaternion operator*(float lhs, const Quaternion& rhs)
{
    return Quaternion(rhs.value_ * lhs);
}


std::ostream& operator<<(std::ostream& stream, const Quaternion& rhs)
{
    stream << rhs.value_;
    return stream;
}



Quaternion slerp(const Quaternion& q0, const Quaternion& q1, float t)
{
    // Only unit quaternions are valid rotations.
    // Normalize to avoid undefined behavior.
    Quaternion v0(q0.normalized());
    Quaternion v1(q1.normalized());

    // Compute the cosine of the angle between the two vectors.
    float dot = v0.dot_vector(v1);

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by
    // reversing one quaternion.
    if(dot < 0.0f)
    {
        v1 = -v1;
        dot = -dot;
    }

    static const float DOT_THRESHOLD = 0.9995f;
    if(dot > DOT_THRESHOLD)
    {
        // If the inputs are too close for comfort, return nlerp
        Quaternion result(v0*(1.f-t) + v1*t);
        result.normalize();
        return result;
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    float theta_0 = acos(dot);        // theta_0 = angle between input vectors
    float theta = theta_0*t;          // theta = angle between v0 and result
    float sin_theta = sin(theta);     // compute this value only once
    float sin_theta_0 = sin(theta_0); // compute this value only once

    float s1 = sin_theta / sin_theta_0;
    float s0 = cos(theta) - dot * s1;  // == sin(theta_0 - theta) / sin(theta_0)

    return (v0 * s0) + (v1 * s1);
}

} // namespace math
} // namespace wcore
