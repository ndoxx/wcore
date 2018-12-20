#include <iostream>
#include <cmath>
#include "math3d.h"

using namespace wcore;
using namespace math;

inline float sign(float x)
{
    return (x>=0.0f)?1.0f:-1.0f;
}

int main()
{
    // Segment defined from start and end points;
    vec3 A(1,-2,1);  // start
    vec3 B(1,12,1); // end

    vec3 AB(B-A);
    vec3 OH(AB.x(),0,AB.z()); // singular if OH = 0 (xB-xA=0 ^ zB-zA=0)
    vec3 v(AB.normalized());
    vec3 w(OH.normalized());

    std::cout << "A: " << A << std::endl;
    std::cout << "B: " << B << std::endl;
    std::cout << "AB: " << AB << std::endl;

    // * Find affine matrix that sends segment [0,1] to [A,B]
    // Translation
    mat4 T;
    T.init_translation(A);
    std::cout << T << std::endl;

    // Scale
    float s = AB.norm();
    mat4 S;
    S.init_diagonal(vec4(s,s,s,1));
    std::cout << S << std::endl;

    // Rotation
    //float theta = asin(AB.y()/s);
    /*mat4 Rz(cos(theta), -sin(theta), 0, 0,
            sin(theta), cos(theta),  0, 0,
            0,          0,           1, 0,
            0,          0,           0, 1);*/

    /*float phi = sign(w.z()) * acos(w.x()); // w.z is the y component of w cross x
    mat4 Ry(cos(phi), 0, -sin(phi), 0,
            0,        1, 0,         0,
            sin(phi), 0, cos(phi),  0,
            0,        0, 0,         1);*/

    float k = AB.y()/s;
    float d = sqrt(1.0f-k*k);
    float l = w.x();
    float e = sign(w.z()) * sqrt(1.0f-l*l);

    /*mat4 Rz(d, -k, 0, 0,
            k, d,  0, 0,
            0, 0,  1, 0,
            0, 0,  0, 1);

    mat4 Ry(l, 0, -e, 0,
            0, 1, 0,  0,
            e, 0, l,  0,
            0, 0, 0,  1);

    mat4 R(Ry*Rz);*/

    mat4 R(l*d, -l*k, -e, 0,
           k,   d,    0,  0,
           e*d, -e*k, l,  0,
           0,   0,    0,  1);

    mat4 TRS(s*l*d, -s*l*k, -s*e, A.x(),
             s*k,   s*d,    0,    A.y(),
             s*e*d, -s*e*k, s*l,  A.z(),
             0,     0,      0,    1);

    std::cout << TRS << std::endl;
    std::cout << TRS*vec3(1,0,0) << std::endl;

    mat4 trans(math::segment_transform(A,B));
    std::cout << trans*vec3(0,0,0) << std::endl;
    std::cout << trans*vec3(1,0,0) << std::endl;


    return 0;
}
