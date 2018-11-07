#include <catch2/catch.hpp>
#include <iostream>
#include "catch_math_common.h"

#define __DEBUG__

#include "transformation.h"

using namespace math;

static const float precision = 1e-4;

TEST_CASE("Initializing transformation object.", "[transf]")
{
    Transformation trf(vec3(1.0, 2.0, 3.0),
                       quat(vec3(0.0, 0.0, 1.0), 30.0)); // == quat(30.0, 0.0, 0.0)

    const quat& q = trf.get_orientation();

    REQUIRE(trf.get_position() == vec3(1.0, 2.0, 3.0));
    REQUIRE(VectorNear(vec4(0.0, 0.0, 0.2588, 0.9659), q.get_as_vec(), precision));
}

TEST_CASE("Getting model matrix.", "[transf]")
{
    Transformation trf(vec3(1.0, 2.0, 3.0),
                       quat(30.0, 0.0, 0.0));

    mat4 M(trf.get_model_matrix());

    mat4 expect(0.8660, -0.5, 0.0, 1.0,
                0.5, 0.8660, 0.0, 2.0,
                0.0, 0.0, 1.0, 3.0,
                0.0, 0.0, 0.0, 1.0);
    REQUIRE(MatrixNear(expect, M, precision));
}

TEST_CASE("Multiplying two Transformations.", "[transf]")
{
    Transformation trf1(vec3(1.0, 2.0, 3.0),
                        quat(30.0, 0.0, 0.0),
                        2.0f);
    Transformation trf2(vec3(-1.0, 5.0, 0.1),
                        quat(0.0, 30.0, 0.0),
                        1.0f);

    Transformation trf12(trf1*trf2);

    vec3 expectT12 = trf1.get_position() + trf2.get_position();
    quat expectQ12 = trf1.get_orientation() * trf2.get_orientation();

    REQUIRE(VectorNear(expectT12, trf12.get_position(), precision));
    REQUIRE(VectorNear(expectQ12.get_as_vec(), trf12.get_orientation().get_as_vec(), precision));
    REQUIRE(FloatNear(2.0f, trf12.get_scale(), precision));
    REQUIRE(trf12.is_scaled());
}

TEST_CASE("Translating object.", "[transf]")
{
    Transformation trf(vec3(12.0, -12.0, 5.0),
                       quat(30.0, 0.0, 0.0));

    SECTION("Vector translation")
    {
        trf.translate(vec3(1.0, 1.0, -5.0));

        vec3 expect(13.0, -11.0, 0.0);
        REQUIRE(VectorNear(expect, trf.get_position(), precision));
    }

    SECTION("Translation along x-axis")
    {
        trf.translate_x(5.0);

        vec3 expect(17.0, -12.0, 5.0);
        REQUIRE(VectorNear(expect, trf.get_position(), precision));
    }

    SECTION("Translation along y-axis")
    {
        trf.translate_y(-5.0);

        vec3 expect(12.0, -17.0, 5.0);
        REQUIRE(VectorNear(expect, trf.get_position(), precision));
    }

    SECTION("Translation along z-axis")
    {
        trf.translate_z(2.0);

        vec3 expect(12.0, -12.0, 7.0);
        REQUIRE(VectorNear(expect, trf.get_position(), precision));
    }
}

TEST_CASE("Rotating object.", "[transf]")
{
    Transformation trf(vec3(0.0, 0.0, 0.0),
                       quat(30.0, 0.0, 0.0));

    trf.rotate(30.0, 0.0, 0.0);

    quat q(trf.get_orientation());
    vec3 expect(60.0, 0.0, 0.0);
    REQUIRE(VectorNear(expect, q.get_euler_angles(), precision));
}

TEST_CASE("Scaling object.", "[transf]")
{
    Transformation trf(vec3(0.0, 0.0, 0.0),
                       quat(30.0, 0.0, 0.0),
                       2.0f);

    REQUIRE(trf.get_scale() == 2.0f);
    REQUIRE(trf.is_scaled());

    trf.translate(1.0, 2.0, 3.0);

    mat4 M(trf.get_model_matrix());

    mat4 expect(2*0.8660, -2*0.5, 0.0, 1.0,
                2*0.5, 2*0.8660, 0.0, 2.0,
                0.0, 0.0, 2*1.0, 3.0,
                0.0, 0.0, 0.0, 1.0);
    REQUIRE(MatrixNear(expect, M, precision));
}
/*
TEST_CASE("Rotating object around a point, given axis of rotation and angle.", "[transf]")
{
    SECTION("Position at origin, rotate around [1,0,0], up axis.")
    {
        Transformation trf(vec3(0.0, 0.0, 0.0),
                           quat(30.0, 0.0, 0.0));

        trf.rotate_around(vec3(1.0, 0.0, 0.0), 60.0);

        REQUIRE(VectorNear(vec3(0.866025, 0, 0.5), trf.get_position(), precision));
    }

    SECTION("Position at origin, rotate around [2,0,0], up axis.")
    {
        Transformation trf;

        trf.rotate_around(vec3(2.0, 0.0, 0.0), 60.0);
        REQUIRE(VectorNear(vec3(2*0.866025, 0, 2*0.5), trf.get_position(), precision));
    }

    SECTION("Position at origin, rotate around [0,0,1], up axis.")
    {
        Transformation trf;

        trf.rotate_around(vec3(0.0, 0.0, 1.0), 60.0);

        REQUIRE(VectorNear(vec3(-0.866025, 0, 0.5), trf.get_position(), precision));
    }

}
*/
