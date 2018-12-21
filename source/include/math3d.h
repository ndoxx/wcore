#ifndef MATH_H
#define MATH_H

#include "math_structures.hpp"

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

void init_rotation_euler(mat4& Matrix, float z, float y, float x);

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

// Returns a matrix that transforms unitary segment along x to any segment starting at
// world position world_start and ending at world_end.
mat4 segment_transform(const vec3& world_start, const vec3& world_end);

// Returns a scale-translation matrix
mat4 scale_translate(const vec3& world_position, float scale);

} // namespace math
} // namespace wcore

#include "quaternion.h"

#endif // MATH_H
