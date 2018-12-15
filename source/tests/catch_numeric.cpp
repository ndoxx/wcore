#include <catch2/catch.hpp>
#include <iostream>
#include <cmath>
#include "numeric.h"
#include "gaussian.h"
#include "catch_math_common.h"

#define NSUBDIV 6

static const float precision = 1e-4;

TEST_CASE("Simpson rule integration of x^2 btw 0 and 1.", "[intg]")
{
    float result = integrate_simpson([&](float x){ return x*x; }, 0.0f, 1.0f, NSUBDIV);

    REQUIRE(FloatNear(1.0f/3.0f, result, precision));
}

TEST_CASE("Simpson rule integration of x^3 btw 0 and 1.", "[intg]")
{
    float result = integrate_simpson([&](float x){ return x*x*x; }, 0.0f, 1.0f, NSUBDIV);

    REQUIRE(FloatNear(1.0f/4.0f, result, precision));
}

TEST_CASE("Simpson rule integration of sin(x) btw 0 and pi/2.", "[intg]")
{
    float result = integrate_simpson([&](float x){ return sin(x); }, 0.0f, M_PI/2.0f, NSUBDIV);

    REQUIRE(FloatNear(1.0f, result, precision));
}

// Values from TRUSTED http://dev.theomader.com/gaussian-kernel-calculator/
TEST_CASE("Gaussian kernel of size 5, sigma=0.7", "[gauss]")
{
    GaussianKernel gkernel(5, 0.7f);

    REQUIRE(FloatNear(gkernel[0], 0.525136f, precision));
    REQUIRE(FloatNear(gkernel[1], 0.221542f, precision));
    REQUIRE(FloatNear(gkernel[2], 0.015890f, precision));
}

TEST_CASE("Gaussian kernel of size 9, sigma=1.8", "[gauss]")
{
    GaussianKernel gkernel(9, 1.8f);

    REQUIRE(FloatNear(gkernel[0], 0.221569f, precision));
    REQUIRE(FloatNear(gkernel[1], 0.190631f, precision));
    REQUIRE(FloatNear(gkernel[2], 0.121403f, precision));
    REQUIRE(FloatNear(gkernel[3], 0.057223f, precision));
    REQUIRE(FloatNear(gkernel[4], 0.019959f, precision));
}

TEST_CASE("Gaussian kernel of size 11, sigma=2.0", "[gauss]")
{
    GaussianKernel gkernel(11, 2.0f);

    REQUIRE(FloatNear(gkernel[0], 0.198596f, precision));
    REQUIRE(FloatNear(gkernel[1], 0.175713f, precision));
    REQUIRE(FloatNear(gkernel[2], 0.121703f, precision));
    REQUIRE(FloatNear(gkernel[3], 0.065984f, precision));
    REQUIRE(FloatNear(gkernel[4], 0.028002f, precision));
    REQUIRE(FloatNear(gkernel[5], 0.009300f, precision));

}
