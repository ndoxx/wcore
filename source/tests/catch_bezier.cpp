#include <catch2/catch.hpp>
#include <iostream>
#include "catch_math_common.h"

#include "bezier.h"

static const float precision = 1e-4;

TEST_CASE("Bezier curve order 3 interpolation.", "[bez]")
{
    Bezier curve(vec3(0.0,0.0,0.0),
                       vec3(1.0,0.2,0.0),
                       vec3(1.0,0.4,1.0),
                       vec3(0.0,0.6,1.0));

    REQUIRE(VectorNear(vec3(0.0,0.0,0.0), curve.interpolate(0.0f), precision));
    REQUIRE(VectorNear(vec3(0.0,0.6,1.0), curve.interpolate(0.9999999f), precision));
    REQUIRE(VectorNear(vec3(0.0,0.6,1.0), curve.interpolate(1.0f), precision));
}

TEST_CASE("Update control point.", "[bez]")
{
    Bezier curve(vec3(0.0,0.0,0.0),
                       vec3(1.0,0.2,0.0),
                       vec3(1.0,0.4,1.0),
                       vec3(0.0,0.6,1.0));

    curve.update_control_point(3, vec3(0.5,1.2,1.0));
    REQUIRE(VectorNear(vec3(0.5,1.2,1.0), curve.interpolate(1.0f), precision));
}

TEST_CASE("De Casteljau's algorithm.", "[bez]")
{
    vec3 p = Bezier::interpolate(0.5,
                        vec3(0.0,0.0,0.0),
                        vec3(1.0,0.2,0.0),
                        vec3(1.0,0.4,1.0),
                        vec3(8.0,0.6,1.0));

    Bezier curve(vec3(0.0,0.0,0.0),
                       vec3(1.0,0.2,0.0),
                       vec3(1.0,0.4,1.0),
                       vec3(8.0,0.6,1.0));
    vec3 expect = curve.interpolate(0.5f);

    REQUIRE(VectorNear(expect, p, precision));
}
