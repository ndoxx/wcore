#include <catch2/catch.hpp>
#include <iostream>
#include "math3d.h"
#include "catch_math_common.h"

using namespace math;

static const float precision = 1e-4;

TEST_CASE("Default ctor is used to create vecs.", "[vec]")
{
    vec2 a2;
    vec3 a3;
    vec4 a4;
    REQUIRE(a2 == vec2(0.0, 0.0));
    REQUIRE(a3 == vec3(0.0, 0.0, 0.0));
    REQUIRE(a4 == vec4(0.0, 0.0, 0.0, 0.0));
}

TEST_CASE("Unique value ctor is used.", "[vec]")
{
    vec2 a2(1.0);
    vec3 a3(1.0);
    vec4 a4(1.0);
    REQUIRE(a2 == vec2(1.0, 1.0));
    REQUIRE(a3 == vec3(1.0, 1.0, 1.0));
    REQUIRE(a4 == vec4(1.0, 1.0, 1.0, 1.0));
}

TEST_CASE("Arglist ctor used.", "[vec]")
{
    SECTION("Just the right amount of args in arglist ctor.")
    {
        vec3 a3(1.0, 2.0, 3.0);
        vec4 a4(1.0, 2.0, 3.0, 4.0);
        REQUIRE(a3 == vec3(1.0, 2.0, 3.0));
        REQUIRE(a4 == vec4(1.0, 2.0, 3.0, 4.0));
    }

    SECTION("Infranumerary args in arglist ctor.")
    {
        vec3 a3(1.0, 2.0);
        vec4 a4(1.0, 2.0, 3.0);
        REQUIRE(a3 == vec3(1.0, 2.0, 0.0));
        REQUIRE(a4 == vec4(1.0, 2.0, 3.0, 0.0));
    }

    SECTION("Supernumerary args in arglist ctor.")
    {
        vec2 a2(1.0, 2.0, 3.0);
        vec3 a3(1.0, 2.0, 3.0, 4.0);
        vec4 a4(1.0, 2.0, 3.0, 4.0, 5.0);
        REQUIRE(a2 == vec2(1.0, 2.0));
        REQUIRE(a3 == vec3(1.0, 2.0, 3.0));
        REQUIRE(a4 == vec4(1.0, 2.0, 3.0, 4.0));
    }
}

TEST_CASE("Copy ctor used.", "[vec]")
{
    SECTION("High dim to low dim initialization.")
    {
        vec3 a3(1.0, 2.0, 3.0);
        vec4 a4(1.0, 2.0, 3.0, 4.0);
        vec2 b2(a3);
        vec2 c2(a4);
        REQUIRE(b2 == vec2(1.0, 2.0));
        REQUIRE(c2 == vec2(1.0, 2.0));
    }

    SECTION("Low dim to high dim initialization.")
    {
        vec2 a2(1.0, 2.0);
        vec2 b2(1.0, 2.0);
        vec3 a3(a2);
        vec4 a4(b2);
        REQUIRE(a3 == vec3(1.0, 2.0, 0.0));
        REQUIRE(a4 == vec4(1.0, 2.0, 0.0, 0.0));
    }

    SECTION("N to N+1 dimension + 1 parameter initialization.")
    {
        vec2 a2(1.0, 2.0);
        vec3 a3(1.0, 2.0, 3.0);
        vec3 b3(a2, 15.28);
        vec4 b4(a3, -22.16);
        REQUIRE(b3 == vec3(1.0, 2.0, 15.28));
        REQUIRE(b4 == vec4(1.0, 2.0, 3.0, -22.16));
    }
}

TEST_CASE("Swapping two vectors.", "[vec]")
{
    vec3 a3(1.0, 2.0, 3.0);
    vec3 b3(4.0, 5.0, 6.0);

    std::swap(a3, b3);

    REQUIRE(a3 == vec3(4.0, 5.0, 6.0));
    REQUIRE(b3 == vec3(1.0, 2.0, 3.0));
}

TEST_CASE("Reference accessor.", "[vec]")
{
    vec2 b2(1.0, 2.0);
    float& b2_1_ref = b2[1];
    b2_1_ref = -42.8;
    REQUIRE(b2 == vec2(1.0, -42.8));
}

TEST_CASE("Adding two vecs.", "[vec]")
{
    vec3 v1(1.0, 2.0, -1.0);
    vec3 v2(2.0, -2.0, 1.0);
    REQUIRE((v1+v2) == vec3(3.0, 0.0, 0.0));
}

TEST_CASE("Subtracting two vecs.", "[vec]")
{
    vec3 v1(1.0, 2.0, -1.0);
    vec3 v2(2.0, -2.0, 1.0);
    REQUIRE((v1-v2) == vec3(-1.0, 4.0, -2.0));
}

TEST_CASE("Hadamard product.", "[vec]")
{
    vec3 v1(1.0, 2.0, -1.0);
    vec3 v2(2.0, -2.0, 1.0);
    REQUIRE((v1*v2) == vec3(2.0, -4.0, -1.0));
}

TEST_CASE("Scalar product.", "[vec]")
{
    vec3 v1(1.0, 2.0, -1.0);
    REQUIRE((v1*2.0f) == vec3(2.0, 4.0, -2.0));
    REQUIRE((3.0f*v1) == vec3(3.0, 6.0, -3.0));
}

TEST_CASE("Scalar division.", "[vec]")
{
    vec3 v1(3.0, 2.0, -1.0);
    REQUIRE((v1/2.0f) == vec3(1.5, 1.0, -0.5));
}

TEST_CASE("Scalar product assignment.", "[vec]")
{
    vec3 v1(1.0, 2.0, -1.0);
    v1 *= 2.0;
    REQUIRE(v1 == vec3(2.0, 4.0, -2.0));
}

TEST_CASE("Scalar division assignment.", "[vec]")
{
    vec3 v1(1.0, 2.0, -1.0);
    v1 /= 2.0;
    REQUIRE(v1 == vec3(0.5, 1.0, -0.5));
}

TEST_CASE("Scalar sum assignment.", "[vec]")
{
    vec3 v1(1.0, 2.0, 3.0);
    vec3 v2(4.1, 5.2, -3.0);
    v1 += v2;
    REQUIRE(v1 == vec3(5.1, 7.2, 0.0));
}

TEST_CASE("Scalar difference assignment.", "[vec]")
{
    vec3 v1(1.0, 2.0, 3.0);
    vec3 v2(2.0, 3.0, -3.0);
    v1 -= v2;
    REQUIRE(v1 == vec3(-1.0, -1.0, 6.0));
}

TEST_CASE("Dot product.", "[vec]")
{
    SECTION("Using member func.")
    {
        vec3 v1(1.0, 2.0, 3.0);
        vec3 v2(2.0, 3.5, -3.0);
        float result = v1.dot(v2);
        REQUIRE(result == 0.0f);
        REQUIRE(result == v2.dot(v1));
    }

    SECTION("Using external func.")
    {
        vec3 v1(1.0, 2.0, 3.0);
        vec3 v2(2.0, 3.5, -3.0);
        float result = dot(v1,v2);
        REQUIRE(result == 0.0f);
        REQUIRE(result == dot(v2, v1));
    }
}

TEST_CASE("Vector product member func.", "[vec]")
{
    SECTION("Using member func.")
    {
        vec3 v1(1.0, 2.0, 3.0);
        vec3 v2(2.0, 3.5, -3.0);
        vec3 result(v1.cross(v2));
        vec3 result_m(v2.cross(v1));
        REQUIRE(result == vec3(-16.5000, 9.0000, -0.5000));
        REQUIRE(result == -result_m);
    }

    SECTION("Using external func.")
    {
        vec3 v1(1.0, 2.0, 3.0);
        vec3 v2(2.0, 3.5, -3.0);
        vec3 result(cross(v1, v2));
        vec3 result_m(cross(v2, v1));
        REQUIRE(result == vec3(-16.5000, 9.0000, -0.5000));
        REQUIRE(result == -result_m);
    }
}

TEST_CASE("Norm computation.", "[vec]")
{
    vec3 v1(1.0, 2.0, 3.0);
    float v1n = v1.norm();
    REQUIRE(FloatNear(sqrt(14.0), v1n, precision));
}

TEST_CASE("Norm squared computation.", "[vec]")
{
    vec3 v1(1.0, 2.0, 3.0);
    float v1n2 = v1.norm2();
    REQUIRE(v1n2 == 14.0f);
}

TEST_CASE("Distance btw 2 points.", "[vec]")
{
    vec3 A(1.0, 1.0, 1.0);
    vec3 B(-1.0, -1.0, -1.0);
    float dist = distance(A, B);
    REQUIRE(FloatNear(sqrt(12.0), dist, precision));
}

TEST_CASE("Distance squared btw 2 points.", "[vec]")
{
    vec3 A(1.0, 1.0, 1.0);
    vec3 B(-1.0, -1.0, -1.0);
    float dist2 = distance2(A, B);
    REQUIRE(dist2 == 12.0f);
}

TEST_CASE("Maximal element in vector.", "[vec]")
{
    vec3 v1(1.0, 5.0, -2.0);
    REQUIRE(v1.max() == 5.0f);
}

TEST_CASE("Minimal element in vector.", "[vec]")
{
    vec3 v1(1.0, 5.0, -2.0);
    REQUIRE(v1.min() == -2.0f);
}

TEST_CASE("Vector rotation: basis.", "[vec]")
{
    SECTION("Basis 90° rotations.")
    {
        vec3 xx(1.0, 0.0, 0.0);
        vec3 yy(0.0, 1.0, 0.0);
        vec3 zz(0.0, 0.0, 1.0);

        vec3 r1(xx.rotated(TORADIANS(90.0), vec3(0, 0, 1)));
        vec3 r2(yy.rotated(TORADIANS(90.0), vec3(1, 0, 0)));
        vec3 r3(zz.rotated(TORADIANS(90.0), vec3(0, 1, 0)));

        REQUIRE(VectorNear(vec3(0.0, 1.0, 0.0), r1, precision));
        REQUIRE(VectorNear(vec3(0.0, 0.0, 1.0), r2, precision));
        REQUIRE(VectorNear(vec3(1.0, 0.0, 0.0), r3, precision));
    }

    SECTION("45° rotation around z-axis.")
    {
        vec3 xx(1.0, 0.0, 0.0);
        vec3 r1(xx.rotated(TORADIANS(45.0), vec3(0, 0, 1)));
        REQUIRE(VectorNear(vec3(0.707107, 0.707107, 0.0), r1, precision));
    }
}

TEST_CASE("Normalization.", "[vec]")
{
    vec4 vv(1.0, 1.0, 1.0, 1.0);
    vv.normalize();
    REQUIRE(VectorNear(vec4(0.25), vv, precision));
}

TEST_CASE("Vector lerp", "[vec]")
{
    SECTION("Using member func.")
    {
        vec3 xx(1.0, 0.0, 0.0);
        vec3 yy(0.0, 1.0, 0.0);
        vec3 lxy(xx.lerp(yy, 0.5));
        REQUIRE(VectorNear(vec3(0.5, 0.5, 0.0), lxy, precision));
    }

    SECTION("Using external func.")
    {
        vec3 v1(1.0, 0.0, 0.0);
        vec3 v2(0.0, 0.0, 2.0);
        vec3 lxy(lerp(v1, v2, 0.5));
        REQUIRE(VectorNear(vec3(0.5, 0.0, 1.0), lxy, precision));
    }
}
