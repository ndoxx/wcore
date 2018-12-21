#include <catch2/catch.hpp>
#include <iostream>
#include "catch_math_common.h"

#include "cspline.h"

static const float precision = 1e-4;

TEST_CASE("CSpline float interpolation.", "[cspline]")
{
    CSpline<float> cspline({0,1}, {1,2}, {0,0});

    float ans = cspline.interpolate(0.5);
    REQUIRE(FloatNear(ans, 1.5, precision));
}
/*
TEST_CASE("CSpline vec3 interpolation.", "[cspline]")
{
    CSpline<vec3> cspline({0,1},
                          {{0,1,0}, {1,0.5,0.5}},
                          {vec3(0), vec3(0)});

    vec3 ans(cspline.interpolate(0.5));
    REQUIRE(VectorNear(ans, vec3(0.5, 0.75, 0.25), precision));
}*/

TEST_CASE("CSpline float interpolation multiple samples.", "[cspline]")
{
    CSpline<float> cspline({0,1,4,8},
                           {1,2,-2,3},
                           {1,-1});

    for(int ii=0; ii<=100; ++ii)
    {
        float x = -1.0f+10*ii/100.0f;
        std::cout << x << " " << cspline.interpolate(x) << std::endl;
    }
}
