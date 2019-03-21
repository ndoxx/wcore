#ifndef MATH_H
#define MATH_H

#include <array>
#include <string>
#include <cstring>

#include "math_structures.hpp"
#include "wtypes.h"

namespace wcore
{

#define ENABLE_DETERMINANT_DISCRIMINATION_OPT
#define ENABLE_AFFINE_MATRIX_INVERSION_OPT

#ifdef ENABLE_DETERMINANT_DISCRIMINATION_OPT
    #define DET_DISCRIMINATION_MIN 1e-5
#endif

#define TORADIANS(A) M_PI*A/180.0f
#define TODEGREES(A) 180.0f*A/M_PI

namespace math
{

typedef std::array<float, 6> extent_t;

// Vector helper funcs
template <unsigned N, typename T>
inline T dot(const vec<N,T>& v1, const vec<N,T>& v2)
{
    return v1.dot(v2);
}

template <unsigned N, typename T>
inline vec<N,T> normalize(const vec<N,T>& v0)
{
    return v0.normalized();
}

template <unsigned N, typename T>
inline vec<N,T> lerp(const vec<N,T>& v1, const vec<N,T>& v2, float param)
{
    return v1.lerp(v2,param);
}

template <unsigned N, typename T>
inline T norm(const vec<N,T>& v)
{
    return v.norm();
}

template <unsigned N, typename T>
inline T norm2(const vec<N,T>& v)
{
    return v.norm2();
}

template <unsigned N, typename T>
inline T distance(const vec<N,T>& v1, const vec<N,T>& v2)
{
    return (v2-v1).norm();
}

template <unsigned N, typename T>
inline T distance2(const vec<N,T>& v1, const vec<N,T>& v2)
{
    return (v2-v1).norm2();
}

template <typename T>
inline vec<3,T> cross(const vec<3,T>& v1, const vec<3,T>& v2)
{
    return v1.cross(v2);
}

template <unsigned N, typename T>
inline mat<N,T> lerp(const mat<N,T>& m1, const mat<N,T>& m2, float param)
{
    return (1.0f-param)*m1 + param*m2;
}

template <unsigned D, typename T>
inline const mat<D,T>& init_diagonal(mat<D,T>& Matrix, const vec<D,T>& diag)
{
    return Matrix.init_diagonal(diag);
}
template <unsigned D, typename T>
inline const mat<D,T>& init_identity(mat<D,T>& Matrix)
{
    return Matrix.init_identity();
}
template <unsigned D, typename T>
inline const mat<D,T>& init_scale(mat<D,T>& Matrix, const vec<D-1,T>& scales)
{
    return Matrix.init_scale(scales);
}

inline void init_translation(mat4& Matrix, const vec3& translation)
{
    Matrix.init_translation(translation);
}

float det(const mat2& Matrix);

float det(const mat3& Matrix);

float det(const mat4& Matrix);

// Make rotation matrix from ZYX-Tait-Bryan angles
void init_rotation_tait_bryan(mat4& Matrix, float z, float y, float x);
inline void init_rotation_tait_bryan(mat4& Matrix, const vec3& angles_zyx)
{
    init_rotation_tait_bryan(Matrix, angles_zyx[0], angles_zyx[1], angles_zyx[2]);
}

// Initialize view matrix, given camera position and Tait-Bryan angles
void init_view_position_angles(mat4& Matrix, const vec3& eye, const vec3& angles);

// Initialize view matrix, given camera position, target position and an up vector
void init_look_at(mat4& Matrix, const vec3& eye, const vec3& target, const vec3& up);

// Perspective projection matrix with symmetric viewing volume
void init_perspective(mat4& Matrix, float fov, float aspectRatio, float zNear, float zFar, bool leftHanded=true);

// Perspective projection matrix with general frustum (OpenGL style)
void init_frustum(mat4& Matrix, const Frustum&, bool leftHanded=true);

// Orthographic projection matrix with general frustum (OpenGL style)
void init_ortho(mat4& Matrix, const Frustum&, bool leftHanded=true);

// Optimized code for 3*3 inverse
bool inverse(const mat3& m, mat3& Inverse);

// Optimized code for 3*3 TRANSPOSED inverse
bool transposed_inverse(const mat3& m, mat3& TInverse);

// Optimized code for AFFINE 4*4 inverse computation
bool inverse_affine(const mat4& m, mat4& Inverse);

// Optimized code for AFFINE 4*4 TRANSPOSED inverse computation
bool transposed_inverse_affine(const mat4& m, mat4& Inverse);

// Optimized code for 4*4 inverse computation
bool inverse(const mat4& m, mat4& Inverse);

// Optimized code for 4*4 TRANSPOSED inverse computation
bool transposed_inverse(const mat4& m, mat4& TInverse);

template <typename T>
T lerp(const T& t1, const T& t2, float param)
{
    //return (1.0f-param)*t1 + param*t2;
    return t1 + param*(t2-t1);
}

void srand_vec3(uint32_t seed=0);
vec3 random_vec3(const extent_t& extent);

// Returns a matrix that transforms unitary segment along x to any segment starting at
// world position world_start and ending at world_end.
mat4 segment_transform(const vec3& world_start, const vec3& world_end);

// Returns a scale-translation matrix
mat4 scale_translate(const vec3& world_position, float scale);

void translate_matrix(mat4& matrix, const vec3& translation);

} // namespace math

template<> std::string to_string(const math::vec2& v);
template<> std::string to_string(const math::vec3& v);
template<> std::string to_string(const math::vec4& v);
template<> std::string to_string(const math::i32vec2& v);
template<> std::string to_string(const math::i32vec3& v);
template<> std::string to_string(const math::i32vec4& v);

template <typename T>
bool str_val(const char* value, T& result)
{
    std::istringstream iss(value);
    return !(iss >> result).fail();
}

// Full specializations
template <>
bool str_val<math::vec<2> >(const char* value, math::vec<2>& result);

template <>
bool str_val<math::vec<3> >(const char* value, math::vec<3>& result);

template <>
bool str_val<math::vec<4> >(const char* value, math::vec<4>& result);

template <>
bool str_val<math::vec<2,uint32_t> >(const char* value, math::vec<2,uint32_t>& result);

template <>
bool str_val<math::vec<3,uint32_t> >(const char* value, math::vec<3,uint32_t>& result);

template <>
bool str_val<math::vec<4,uint32_t> >(const char* value, math::vec<4,uint32_t>& result);

template <>
bool str_val<bool>(const char* value, bool& result);

} // namespace wcore

#include "quaternion.h"

#endif // MATH_H
