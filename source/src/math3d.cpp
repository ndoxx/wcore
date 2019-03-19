#include <random>
#include "math3d.h"

namespace wcore
{
namespace math
{

/*
    The following functions use direct access to matrix elements.
    Matrices are column major in memory [[col1],[col2],...,[colN]]
    so we need to initialize elements in this order so as to minimize
    cash misses. So a line of initialization like this one:
        M[0]  = 1.0f;    M[1]  = 0.0f;    M[2]  = 0.0f ;  M[3]  = 0.0f;
    would initialize the first COLUMN of a 4x4 matrix.
*/


void init_rotation_euler(mat4& Matrix, float z, float y, float x)
{
    float a1 = z;
    float a2 = y;
    float a3 = x;

    Matrix = mat4(cos(a1)*cos(a2), cos(a1)*sin(a2)*sin(a3)-cos(a3)*sin(a1), sin(a1)*sin(a3)+cos(a1)*cos(a3)*sin(a2), 0.f,
                  cos(a2)*sin(a1), cos(a1)*cos(a3)+sin(a1)*sin(a2)*sin(a3), cos(a3)*sin(a1)*sin(a2)-cos(a1)*sin(a3), 0.f,
                  -sin(a2),        cos(a2)*sin(a3),                         cos(a2)*cos(a3),                         0.f,
                  0.f,             0.f,                                     0.f,                                     1.f);
}

void init_look_at(mat4& Matrix, const vec3& eye, const vec3& target, const vec3& up)
{
    vec3 zaxis = (eye - target).normalized();     // "forward -z" vector.
    vec3 xaxis = (cross(up, zaxis)).normalized(); // "right" vector.
    vec3 yaxis = cross(zaxis, xaxis);             // "up" vector.

    vec3 trans(-xaxis.dot(eye), -yaxis.dot(eye), -zaxis.dot(eye));

    Matrix[0] = xaxis.x();  Matrix[4] = xaxis.y();  Matrix[8]  = xaxis.z(); Matrix[12] = trans[0];
    Matrix[1] = yaxis.x();  Matrix[5] = yaxis.y();  Matrix[9]  = yaxis.z(); Matrix[13] = trans[1];
    Matrix[2] = zaxis.x();  Matrix[6] = zaxis.y();  Matrix[10] = zaxis.z(); Matrix[14] = trans[2];
    Matrix[3] = 0.0f;       Matrix[7] = 0.0f;       Matrix[11] = 0.0f;      Matrix[15] = 1.0f;
}

void init_perspective(mat4& Matrix, float fov, float aspectRatio, float zNear, float zFar, bool leftHanded)
{
    float yScale = 1.0f / tan(TORADIANS(fov/2));
    // NDC left handed for OpenGL
    float xScale = (leftHanded ? 1 : -1 ) * yScale / aspectRatio;
    float zRange = zFar - zNear;
    float p1 = -(zNear+zFar)/zRange;
    float p2 = -2.0*zFar*zNear/zRange;
    float ll = (leftHanded ? -1 : 1 ); //-1 for Opengl

    Matrix[0] = xScale; Matrix[4] = 0.0f;   Matrix[8]  = 0.0f; Matrix[12] = 0.0f;
    Matrix[1] = 0.0f;   Matrix[5] = yScale; Matrix[9]  = 0.0f; Matrix[13] = 0.0f;
    Matrix[2] = 0.0f;   Matrix[6] = 0.0f;   Matrix[10] = p1;   Matrix[14] = ll;
    Matrix[3] = 0.0f;   Matrix[7] = 0.0f;   Matrix[11] = p2;   Matrix[15] = 0.0f;
}

void init_frustum(mat4& Matrix, const Frustum& f, bool leftHanded)
{
    float xScale = 2*f.n/f.w * (leftHanded ? 1 : -1 );
    float yScale = 2*f.n/f.h;
    float zScale = -(f.f+f.n)/f.d;
    float zx = (f.r+f.l)/f.w;
    float zy = (f.t+f.b)/f.h;
    float zp = -2*f.f*f.n/f.d;
    float ll = (leftHanded ? -1 : 1 ); //-1 for Opengl

    Matrix[0] = xScale; Matrix[4] = 0.0f;   Matrix[8]  = zx;     Matrix[12] = 0.0f;
    Matrix[1] = 0.0f;   Matrix[5] = yScale; Matrix[9]  = zy;     Matrix[13] = 0.0f;
    Matrix[2] = 0.0f;   Matrix[6] = 0.0f;   Matrix[10] = zScale; Matrix[14] = zp;
    Matrix[3] = 0.0f;   Matrix[7] = 0.0f;   Matrix[11] = ll;     Matrix[15] = 0.0f;
}

void init_ortho(mat4& Matrix, const Frustum& f, bool leftHanded)
{
    float tx = -(f.r+f.l)/f.w;
    float ty = -(f.t+f.b)/f.h;
    float tz = -(f.f+f.n)/f.d;

    Matrix[0] = 2.0f/f.w; Matrix[4] = 0.0f;     Matrix[8]  = 0.0f;     Matrix[12] = tx;
    Matrix[1] = 0.0f;     Matrix[5] = 2.0f/f.h; Matrix[9]  = 0.0f;     Matrix[13] = ty;
    Matrix[2] = 0.0f;     Matrix[6] = 0.0f;     Matrix[10] = -2.0/f.d; Matrix[14] = tz;
    Matrix[3] = 0.0f;     Matrix[7] = 0.0f;     Matrix[11] = 0.0f;     Matrix[15] = 1.0f;
}

bool inverse(const mat3& m, mat3& Inverse)
{
    float det;
    Inverse[0] = m[4]*m[8] - m[7]*m[5];
    Inverse[3] = m[5]*m[6] - m[3]*m[8];
    Inverse[6] = m[3]*m[7] - m[6]*m[4];
    det = m[0] * Inverse[0] + m[1] * Inverse[3] + m[2] * Inverse[6];

    #ifdef ENABLE_DETERMINANT_DISCRIMINATION_OPT
    if (std::abs(det) > float(DET_DISCRIMINATION_MIN))
    #else
    if (det != 0.0)
    #endif
    {
        det = 1.0/det;

        Inverse[1] = m[2]*m[7] - m[1]*m[8];
        Inverse[2] = m[1]*m[5] - m[2]*m[4];

        Inverse[4] = m[0]*m[8] - m[2]*m[6];
        Inverse[5] = m[3]*m[2] - m[0]*m[5];

        Inverse[7] = m[6]*m[1] - m[0]*m[7];
        Inverse[8] = m[0]*m[4] - m[3]*m[1];

        for (unsigned i = 0; i < 9; ++i)
            Inverse[i] = Inverse[i] * det;
        return true;
    }
    return false;
}

// Optimized code for 3*3 TRANSPOSED inverse
bool transposed_inverse(const mat3& m, mat3& TInverse)
{
    float det;

    TInverse[0] =  (m[4]*m[8] - m[7]*m[5]);
    TInverse[1] = -(m[3]*m[8] - m[5]*m[6]);
    TInverse[2] =  (m[3]*m[7] - m[6]*m[4]);

    det = m[0] * TInverse[0] + m[1] * TInverse[1] + m[2] * TInverse[2];

    #ifdef ENABLE_DETERMINANT_DISCRIMINATION_OPT
    if (std::abs(det) > float(DET_DISCRIMINATION_MIN))
    #else
    if (det != 0.0)
    #endif
    {
        det = 1.0/det;

        TInverse[3] = -(m[1]*m[8] - m[2]*m[7]);
        TInverse[6] =  (m[1]*m[5] - m[2]*m[4]);

        TInverse[4] =  (m[0]*m[8] - m[2]*m[6]);
        TInverse[7] = -(m[0]*m[5] - m[3]*m[2]);

        TInverse[5] = -(m[0]*m[7] - m[6]*m[1]);
        TInverse[8] =  (m[0]*m[4] - m[3]*m[1]);

        for (unsigned i = 0; i < 9; ++i)
            TInverse[i] = TInverse[i] * det;
        return true;
    }
    return false;
}

// Optimized code for AFFINE 4*4 inverse computation
bool inverse_affine(const mat4& m, mat4& Inverse)
{
    mat3 submat(m.submatrix(3,3));
    mat3 submatInv;
    vec3 trans0(m[12],m[13],m[14]);
    if(!inverse(submat,submatInv)) return false;
    vec3 trans = -1.0*(submatInv*trans0);

    Inverse[0] = submatInv[0]; Inverse[4] = submatInv[3]; Inverse[8] = submatInv[6];   Inverse[12] = trans[0];
    Inverse[1] = submatInv[1]; Inverse[5] = submatInv[4]; Inverse[9] = submatInv[7];   Inverse[13] = trans[1];
    Inverse[2] = submatInv[2]; Inverse[6] = submatInv[5]; Inverse[10] = submatInv[8];  Inverse[14] = trans[2];
    Inverse[3] = 0.0f;         Inverse[7] = 0.0f;         Inverse[11] = 0.0f;          Inverse[15] = 1.0f;

    return true;
}

// Optimized code for AFFINE 4*4 TRANSPOSED inverse computation
bool transposed_inverse_affine(const mat4& m, mat4& Inverse)
{
    mat3 submat(m.submatrix(3,3));
    mat3 submatInv;
    vec3 trans0(m[12],m[13],m[14]);
    if(!inverse(submat,submatInv)) return false;
    vec3 trans = -1.0*(submatInv*trans0);

    Inverse[0] = submatInv[0]; Inverse[4] = submatInv[1]; Inverse[8] = submatInv[2];   Inverse[12] = 0.0f;
    Inverse[1] = submatInv[3]; Inverse[5] = submatInv[4]; Inverse[9] = submatInv[5];   Inverse[13] = 0.0f;
    Inverse[2] = submatInv[6]; Inverse[6] = submatInv[7]; Inverse[10] = submatInv[8];  Inverse[14] = 0.0f;
    Inverse[3] = trans[0];     Inverse[7] = trans[1];     Inverse[11] = trans[2];      Inverse[15] = 1.0f;
    return true;
}

// Optimized code for 4*4 inverse computation
bool inverse(const mat4& m, mat4& Inverse)
{
    #ifdef ENABLE_AFFINE_MATRIX_INVERSION_OPT
    if(m.is_affine())
    {
        return inverse_affine(m,Inverse);
    }
    #endif

    float det;

    Inverse[0]  = m[5]*(m[10]*m[15] - m[11]*m[14]) - m[9]*(m[6]*m[15] -
                  m[7]*m[14]) - m[13]*(m[7]*m[10] - m[6]*m[11]);
    Inverse[4]  = m[4]*(m[11]*m[14] - m[10]*m[15]) - m[8]*(m[7]*m[14] -
                  m[6]*m[15]) - m[12]*(m[6]*m[11] - m[7]*m[10]);
    Inverse[8]  = m[4]*(m[ 9]*m[15] - m[11]*m[13]) - m[8]*(m[5]*m[15] -
                  m[7]*m[13]) - m[12]*(m[7]*m[ 9] - m[5]*m[11]);
    Inverse[12] = m[4]*(m[10]*m[13] - m[ 9]*m[14]) - m[8]*(m[6]*m[13] -
                  m[5]*m[14]) - m[12]*(m[5]*m[10] - m[6]*m[ 9]);

    det = m[0] * Inverse[0] + m[1] * Inverse[4] + m[2] * Inverse[8] + m[3] * Inverse[12];

    #ifdef ENABLE_DETERMINANT_DISCRIMINATION_OPT
    if (std::abs(det) > float(DET_DISCRIMINATION_MIN))
    #else
    if (det != 0.0)
    #endif
    {
        det = 1.0/det;

        Inverse[1]  = m[1]*(m[11]*m[14] - m[10]*m[15]) - m[9]*(m[3]*m[14] -
                      m[2]*m[15]) - m[13]*(m[2]*m[11] - m[3]*m[10]);
        Inverse[2]  = m[1]*(m[ 6]*m[15] - m[ 7]*m[14]) - m[5]*(m[2]*m[15] -
                      m[3]*m[14]) - m[13]*(m[3]*m[ 6] - m[2]*m[ 7]);
        Inverse[3]  = m[1]*(m[ 7]*m[10] - m[ 6]*m[11]) - m[5]*(m[3]*m[10] -
                      m[2]*m[11]) - m[ 9]*(m[2]*m[ 7] - m[3]*m[ 6]);

        Inverse[5]  = m[0]*(m[10]*m[15] - m[11]*m[14]) - m[8]*(m[2]*m[15] -
                      m[3]*m[14]) - m[12]*(m[3]*m[10] - m[2]*m[11]);
        Inverse[6]  = m[0]*(m[ 7]*m[14] - m[ 6]*m[15]) - m[4]*(m[3]*m[14] -
                      m[2]*m[15]) - m[12]*(m[2]*m[ 7] - m[3]*m[ 6]);
        Inverse[7]  = m[0]*(m[ 6]*m[11] - m[ 7]*m[10]) - m[4]*(m[2]*m[11] -
                      m[3]*m[10]) - m[ 8]*(m[3]*m[ 6] - m[2]*m[ 7]);

        Inverse[9]  = m[0]*(m[11]*m[13] - m[ 9]*m[15]) - m[8]*(m[3]*m[13] -
                      m[1]*m[15]) - m[12]*(m[1]*m[11] - m[3]*m[ 9]);
        Inverse[10] = m[0]*(m[ 5]*m[15] - m[ 7]*m[13]) - m[4]*(m[1]*m[15] -
                      m[3]*m[13]) - m[12]*(m[3]*m[ 5] - m[1]*m[ 7]);
        Inverse[11] = m[0]*(m[ 7]*m[ 9] - m[ 5]*m[11]) - m[4]*(m[3]*m[ 9] -
                      m[1]*m[11]) - m[ 8]*(m[1]*m[ 7] - m[3]*m[ 5]);

        Inverse[13] = m[0]*(m[ 9]*m[14] - m[10]*m[13]) - m[8]*(m[1]*m[14] -
                      m[2]*m[13]) - m[12]*(m[2]*m[ 9] - m[1]*m[10]);
        Inverse[14] = m[0]*(m[ 6]*m[13] - m[ 5]*m[14]) - m[4]*(m[2]*m[13] -
                      m[1]*m[14]) - m[12]*(m[1]*m[ 6] - m[2]*m[ 5]);
        Inverse[15] = m[0]*(m[ 5]*m[10] - m[ 6]*m[ 9]) - m[4]*(m[1]*m[10] -
                      m[2]*m[ 9]) - m[ 8]*(m[2]*m[ 5] - m[1]*m[ 6]);

        for (unsigned i = 0; i < 16; ++i)
            Inverse[i] = Inverse[i] * det;
        return true;
    }
    return false;
}

// Optimized code for 4*4 TRANSPOSED inverse computation
bool transposed_inverse(const mat4& m, mat4& TInverse)
{
    #ifdef ENABLE_AFFINE_MATRIX_INVERSION_OPT
    if(m.is_affine())
    {
        return transposed_inverse_affine(m,TInverse);
    }
    #endif

    float det;

    TInverse[0] = m[5]*(m[10]*m[15] - m[11]*m[14]) - m[9]*(m[6]*m[15] -
                  m[7]*m[14]) - m[13]*(m[7]*m[10] - m[6]*m[11]);
    TInverse[1] = m[4]*(m[11]*m[14] - m[10]*m[15]) - m[8]*(m[7]*m[14] -
                  m[6]*m[15]) - m[12]*(m[6]*m[11] - m[7]*m[10]);
    TInverse[2] = m[4]*(m[ 9]*m[15] - m[11]*m[13]) - m[8]*(m[5]*m[15] -
                  m[7]*m[13]) - m[12]*(m[7]*m[ 9] - m[5]*m[11]);
    TInverse[3] = m[4]*(m[10]*m[13] - m[ 9]*m[14]) - m[8]*(m[6]*m[13] -
                  m[5]*m[14]) - m[12]*(m[5]*m[10] - m[6]*m[ 9]);

    det = m[0] * TInverse[0] + m[1] * TInverse[1] + m[2] * TInverse[2] + m[3] * TInverse[3];

    #ifdef ENABLE_DETERMINANT_DISCRIMINATION_OPT
    if (std::abs(det) > float(DET_DISCRIMINATION_MIN))
    #else
    if (det != 0.0)
    #endif
    {
        det = 1.0/det;

        TInverse[4]  = m[1]*(m[11]*m[14] - m[10]*m[15]) - m[9]*(m[3]*m[14] -
                       m[2]*m[15]) - m[13]*(m[2]*m[11] - m[3]*m[10]);
        TInverse[8]  = m[1]*(m[ 6]*m[15] - m[ 7]*m[14]) - m[5]*(m[2]*m[15] -
                       m[3]*m[14]) - m[13]*(m[3]*m[ 6] - m[2]*m[ 7]);
        TInverse[12] = m[1]*(m[ 7]*m[10] - m[ 6]*m[11]) - m[5]*(m[3]*m[10] -
                       m[2]*m[11]) - m[ 9]*(m[2]*m[ 7] - m[3]*m[ 6]);

        TInverse[5]  = m[0]*(m[10]*m[15] - m[11]*m[14]) - m[8]*(m[2]*m[15] -
                       m[3]*m[14]) - m[12]*(m[3]*m[10] - m[2]*m[11]);
        TInverse[9]  = m[0]*(m[ 7]*m[14] - m[ 6]*m[15]) - m[4]*(m[3]*m[14] -
                       m[2]*m[15]) - m[12]*(m[2]*m[ 7] - m[3]*m[ 6]);
        TInverse[13] = m[0]*(m[ 6]*m[11] - m[ 7]*m[10]) - m[4]*(m[2]*m[11] -
                       m[3]*m[10]) - m[ 8]*(m[3]*m[ 6] - m[2]*m[ 7]);

        TInverse[6]  = m[0]*(m[11]*m[13] - m[ 9]*m[15]) - m[8]*(m[3]*m[13] -
                       m[1]*m[15]) - m[12]*(m[1]*m[11] - m[3]*m[ 9]);
        TInverse[10] = m[0]*(m[ 5]*m[15] - m[ 7]*m[13]) - m[4]*(m[1]*m[15] -
                       m[3]*m[13]) - m[12]*(m[3]*m[ 5] - m[1]*m[ 7]);
        TInverse[14] = m[0]*(m[ 7]*m[ 9] - m[ 5]*m[11]) - m[4]*(m[3]*m[ 9] -
                       m[1]*m[11]) - m[ 8]*(m[1]*m[ 7] - m[3]*m[ 5]);

        TInverse[7]  = m[0]*(m[ 9]*m[14] - m[10]*m[13]) - m[8]*(m[1]*m[14] -
                       m[2]*m[13]) - m[12]*(m[2]*m[ 9] - m[1]*m[10]);
        TInverse[11] = m[0]*(m[ 6]*m[13] - m[ 5]*m[14]) - m[4]*(m[2]*m[13] -
                       m[1]*m[14]) - m[12]*(m[1]*m[ 6] - m[2]*m[ 5]);
        TInverse[15] = m[0]*(m[ 5]*m[10] - m[ 6]*m[ 9]) - m[4]*(m[1]*m[10] -
                       m[2]*m[ 9]) - m[ 8]*(m[2]*m[ 5] - m[1]*m[ 6]);


        for (unsigned i = 0; i < 16; ++i)
            TInverse[i] = TInverse[i] * det;
        return true;
    }
    return false;
}

float det(const mat2& m)
{
    return m[0]*m[3]-m[1]*m[2];
}

float det(const mat3& m)
{
    float c0 = m[4]*m[8]-m[5]*m[7];
    float c3 = m[1]*m[8]-m[2]*m[7];
    float c6 = m[1]*m[5]-m[2]*m[4];
    return m[0]*c0-m[3]*c3+m[6]*c6;
}

float det(const mat4& m)
{
    float d1 = m[10]*m[15]-m[11]*m[14];
    float d2 = m[6]*m[15]-m[7]*m[14];
    float d3 = m[6]*m[11]-m[7]*m[10];
    float d4 = m[2]*m[15]-m[3]*m[14];
    float d5 = m[2]*m[11]-m[3]*m[10];
    float d6 = m[2]*m[7]-m[3]*m[6];

    float c0  = m[5]*d1 - m[9]*d2 + m[13]*d3;
    float c4  = m[1]*d1 - m[9]*d4 + m[13]*d5;
    float c8  = m[1]*d2 - m[5]*d4 + m[13]*d6;
    float c12 = m[1]*d3 - m[5]*d5 + m[9]*d6;

    return m[0]*c0-m[4]*c4+m[8]*c8-m[12]*c12;
}

mat4 segment_transform(const vec3& world_start, const vec3& world_end)
{
    vec3 AB(world_end-world_start);
    float s = AB.norm(); // scale is just the length of input segment

    // If line is vertical, general transformation is singular (OH==0)
    // Rotation is around z-axis only and with angle pi/2
    if(fabs(AB.x())<0.0001f && fabs(AB.z())<0.0001f)
    {
        return mat4(0, -s, 0, world_start.x(),
                    s, 0,  0, world_start.y(),
                    0, 0,  s, world_start.z(),
                    0, 0,  0, 1);
    }
    // General transformation
    else
    {
        vec3 OH(AB.x(),0,AB.z()); // project AB on xz plane
        vec3 w(OH.normalized());  // unit vector along AB
        float k = AB.y()/s;       // = sin(theta) (theta angle around z-axis)
        float d = sqrt(1.0f-k*k); // = cos(theta)
        float l = w.x();          // = cos(phi) (phi angle around y-axis)
        float e = ((w.z()>=0.0f)?1.0f:-1.0f) * sqrt(1.0f-l*l); // = sin(phi)

        // Return pre-combined product of T*R*S matrices
        return mat4(s*l*d, -s*l*k, -s*e, world_start.x(),
                    s*k,   s*d,    0,    world_start.y(),
                    s*e*d, -s*e*k, s*l,  world_start.z(),
                    0,     0,      0,    1);
    }
}

mat4 scale_translate(const vec3& world_position, float scale)
{
    mat4 T,S;
    math::init_translation(T, world_position);
    math::init_scale(S, vec3(scale));
    return T*S;
}

void translate_matrix(mat4& matrix, const vec3& translation)
{
    for(unsigned ii=0; ii<3; ++ii)
        matrix[12+ii] += translation[ii];
}


static std::mt19937 genv3;

void srand_vec3(uint32_t seed)
{
    genv3.seed(seed);
}

vec3 random_vec3(const extent_t& extent)
{
    std::uniform_real_distribution<float> dis_x(extent[0], extent[1]);
    std::uniform_real_distribution<float> dis_y(extent[2], extent[3]);
    std::uniform_real_distribution<float> dis_z(extent[4], extent[5]);
    return vec3(dis_x(genv3),dis_y(genv3),dis_z(genv3));
}

} // namespace math

template<>
std::string to_string<math::vec2>(const math::vec2& v)
{
    return "(" + std::to_string(v.x()) + "," + std::to_string(v.y()) + ")";
}

template<>
std::string to_string<math::vec3>(const math::vec3& v)
{
    return "(" + std::to_string(v.x()) + "," + std::to_string(v.y()) + "," + std::to_string(v.z()) + ")";
}

template<>
std::string to_string<math::vec4>(const math::vec4& v)
{
    return "(" + std::to_string(v.x()) + "," + std::to_string(v.y()) + "," + std::to_string(v.z()) + "," + std::to_string(v.w()) + ")";
}

template<>
std::string to_string<math::i32vec2>(const math::i32vec2& v)
{
    return "(" + std::to_string(v.x()) + "," + std::to_string(v.y()) + ")";
}

template<>
std::string to_string<math::i32vec3>(const math::i32vec3& v)
{
    return "(" + std::to_string(v.x()) + "," + std::to_string(v.y()) + "," + std::to_string(v.z()) + ")";
}

template<>
std::string to_string<math::i32vec4>(const math::i32vec4& v)
{
    return "(" + std::to_string(v.x()) + "," + std::to_string(v.y()) + "," + std::to_string(v.z()) + "," + std::to_string(v.w()) + ")";
}

template <>
bool str_val<math::vec<2> >(const char* value, math::vec<2>& result)
{
    return sscanf(value, "(%f,%f)", &result[0], &result[1]) > 0;
}

template <>
bool str_val<math::vec<3> >(const char* value, math::vec<3>& result)
{
    return sscanf(value, "(%f,%f,%f)", &result[0], &result[1], &result[2]) > 0;
}

template <>
bool str_val<math::vec<4> >(const char* value, math::vec<4>& result)
{
    return sscanf(value, "(%f,%f,%f,%f)", &result[0], &result[1], &result[2], &result[3]) > 0;
}

template <>
bool str_val<math::vec<2,uint32_t> >(const char* value, math::vec<2,uint32_t>& result)
{
    return sscanf(value, "(%d,%d)", &result[0], &result[1]) > 0;
}

template <>
bool str_val<math::vec<3,uint32_t> >(const char* value, math::vec<3,uint32_t>& result)
{
    return sscanf(value, "(%d,%d,%d)", &result[0], &result[1], &result[2]) > 0;
}

template <>
bool str_val<math::vec<4,uint32_t> >(const char* value, math::vec<4,uint32_t>& result)
{
    return sscanf(value, "(%d,%d,%d,%d)", &result[0], &result[1], &result[2], &result[3]) > 0;
}

template <>
bool str_val<bool>(const char* value, bool& result)
{
    result = !strcmp(value, "true");
    return true;
}

} // namespace wcore
