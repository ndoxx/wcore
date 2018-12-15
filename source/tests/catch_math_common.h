#ifndef CATCH_MATH_COMMON_H
#define CATCH_MATH_COMMON_H

#include "math3d.h"

using namespace wcore::math;

static bool FloatNear(float expected, float result, float delta)
{
    float err = expected - result;

    if (fabs(err)<delta)
        return true;

    std::cout << "Floats differ by more than " << delta
              << std::endl << "EXPECTED: " << expected << " GOT: " << result;
    return false;
}

static bool VectorNear(const vec3& expected, const vec3& result, float delta)
{
    vec3 err(expected-result);
    bool less_than_delta = true;
    for (unsigned ii = 0; ii < 3; ++ii)
        less_than_delta &= (err[ii]<delta);

    if (less_than_delta)
        return true;

    std::cout << "Vectors differ by more than " << delta
              << std::endl << "EXPECTED: " << expected << " GOT: " << result;
    return false;
}

static bool VectorNear(const vec4& expected, const vec4& result, float delta)
{
    vec4 err(expected-result);
    bool less_than_delta = true;
    for (unsigned ii = 0; ii < 4; ++ii)
        less_than_delta &= (err[ii]<delta);

    if (less_than_delta)
        return true;

    std::cout << "Vectors differ by more than " << delta
              << std::endl << "EXPECTED: " << expected << " GOT: " << result;
    return false;
}

static bool MatrixNear(const mat3& expected, const mat3& result, float delta)
{
    mat3 err(expected-result);
    bool less_than_delta = true;
    for (unsigned ii = 0; ii < 9; ++ii)
        less_than_delta &= (err[ii]<delta);

    if (less_than_delta)
        return true;

    std::cout << "Matrices differ by more than " << delta
              << std::endl << "EXPECTED: " << std::endl
              << expected << " GOT: " << std::endl << result;
    return false;
}

static bool MatrixNear(const mat4& expected, const mat4& result, float delta)
{
    mat4 err(expected-result);
    bool less_than_delta = true;
    for (unsigned ii = 0; ii < 16; ++ii)
        less_than_delta &= (err[ii]<delta);

    if (less_than_delta)
        return true;

    std::cout << "Matrices differ by more than " << delta
              << std::endl << "EXPECTED: " << std::endl
              << expected << " GOT: " << std::endl << result;
    return false;
}


#endif // CATCH_MATH_COMMON_H
