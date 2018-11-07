#include <catch2/catch.hpp>
#include <iostream>
#include "catch_math_common.h"

#define __DEBUG__

#include "camera.h"

using namespace math;

static const float precision = 1e-4;

TEST_CASE("Building default camera.", "[cam]")
{
    Camera cam(1024, 768);
    mat4 proj = cam.get_projection_matrix();

    float aspect = 1024.0f/768.0f;
    REQUIRE(FloatNear(0.5f/aspect, proj[0], precision));
    REQUIRE(FloatNear(0.5f/1.0f, proj[5], precision));
    REQUIRE(FloatNear(-(100.0f+0.5f)/(100.0f-0.5f), proj[10], precision));
    REQUIRE(FloatNear(-1.0f, proj[11], precision));
    REQUIRE(FloatNear(-2.0f*(100.0f*0.5f)/(100.0f-0.5f), proj[14], precision));
}
