#ifndef QUATERNION_H
#define QUATERNION_H

#define TORADIANS(A) M_PI*A/180.0f
#define TODEGREES(A) 180.0f*A/M_PI

#include "math_structures.hpp"

namespace wcore
{
namespace math
{

class Quaternion{
public:
    Quaternion();
    Quaternion(float v);
    Quaternion(float x, float y, float z, float w);
    // ZYX convention (phi=rotZ, theta=rotY, psi=rotX)
    Quaternion(float phi, float theta, float psi);
    Quaternion(const vec3& axis, float angle);
    Quaternion(const vec4& components);
    Quaternion(const vec3& components);
    //Quaternion(const Quaternion& other);
    //Quaternion(Quaternion&& other) noexcept;
    ~Quaternion();

    void init_tait_bryan(float phi, float theta, float psi);
    void init_axis_angle(const vec3& axis, float angle);
    void init_lookat(const vec3& source, const vec3& destination);

    void normalize(float tolerance=NORM_TOL);
    void conjugate();
    float dot_vector(const Quaternion& other);
    inline Quaternion normalized(float tolerance=NORM_TOL) const;
    Quaternion get_conjugate() const;
    Quaternion get_inverse() const;
    inline void set_identity() { value_[0]=0.0f; value_[1]=0.0f; value_[2]=0.0f; value_[3]=1.0f; }

    vec3 rotate(const vec3& vector) const;
    vec3 rotate_inverse(const vec3& vector) const;
    vec3 get_euler_angles(bool degrees=true);
    vec4 get_axis_angle(bool degrees=true);
    mat4 get_rotation_matrix() const;
    inline const vec4& get_as_vec() const {return value_;}

    inline float& operator[](unsigned index)       {return value_[index];}
    inline float  operator[](unsigned index) const {return value_[index];}
    //Quaternion&   operator= (const Quaternion& rhs);
    Quaternion&   operator*=(const Quaternion& rhs);
    Quaternion&   operator+=(const Quaternion& rhs);
    Quaternion    operator-() const;

    friend Quaternion operator+(const Quaternion& lhs, const Quaternion& rhs);
    friend Quaternion operator-(const Quaternion& lhs, const Quaternion& rhs);
    friend Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);
    friend Quaternion operator/(const Quaternion& lhs, const Quaternion& rhs);
    friend Quaternion operator*(const Quaternion& lhs, float rhs);
    friend Quaternion operator*(float lhs, const Quaternion& rhs);
    friend std::ostream& operator<<(std::ostream& stream, const Quaternion& rhs);

private:
    vec4 value_;

    static const float NORM_TOL;
};

inline Quaternion Quaternion::normalized(float tolerance) const
{
    Quaternion result(*this);
    result.normalize();
    return result;
}

using quat = Quaternion;

extern Quaternion slerp(const Quaternion& q0, const Quaternion& q1, float t);

} // namespace math
} // namespace wcore
#endif // QUATERNION_H
