#include <catch2/catch.hpp>
#include <iostream>
#include "math3d.h"
#include "quaternion.h"
#include "catch_math_common.h"

using namespace math;

static const float precision = 1e-4;


TEST_CASE("Default ctor is used to create quat.", "[quat]")
{
    quat qq;
    REQUIRE(vec4(0.0, 0.0, 0.0, 1.0) == qq.get_as_vec());
}

TEST_CASE("Component ctor is used.", "[quat]")
{
    quat qq(1.0, 2.0, 3.0, 4.0);
    REQUIRE(vec4(1.0, 2.0, 3.0, 4.0) == qq.get_as_vec());
}

TEST_CASE("Euler angles ctor is used.", "[quat]")
{
    quat qx(30.0, 0.0, 0.0);
    quat qy(0.0, 30.0, 0.0);
    quat qz(0.0, 0.0, 30.0);
    quat qxyz(30.0, -25.2, 42.5);
    // MATLAB: eul2quat([pi*30/180, 0, 0])
    REQUIRE(VectorNear(vec4(0.0, 0.0, 0.2588, 0.9659), qx.get_as_vec(), precision));
    // MATLAB: eul2quat([0, pi*30/180, 0])
    REQUIRE(VectorNear(vec4(0.0, 0.2588, 0.0, 0.9659), qy.get_as_vec(), precision));
    // MATLAB: eul2quat([0, 0, pi*30/180])
    REQUIRE(VectorNear(vec4(0.2588, 0.0, 0.0, 0.9659), qz.get_as_vec(), precision));
    // MATLAB: eul2quat([30.0 -25.2 42.5]*(pi/180.0))
    REQUIRE(VectorNear(vec4(0.3943, -0.1048, 0.3118, 0.8581), qxyz.get_as_vec(), precision));
}

TEST_CASE("Axis/Angle ctor is used.", "[quat]")
{
    quat qx(vec3(1.0, 0.0, 0.0), 45.0);
    quat qy(vec3(0.0, 1.0, 0.0), 45.0);
    quat qz(vec3(0.0, 0.0, 1.0), 45.0);
    quat qxyz(vec3(0.5, 1.0, -1.0), 23.0);
    // MATLAB: axang2quat([1 0 0 pi/4])
    REQUIRE(VectorNear(vec4(0.3827, 0.0, 0.0, 0.9239), qx.get_as_vec(), precision));
    // MATLAB: axang2quat([0 1 0 pi/4])
    REQUIRE(VectorNear(vec4(0.0, 0.3827, 0.0, 0.9239), qy.get_as_vec(), precision));
    // MATLAB: axang2quat([0 0 1 pi/4])
    REQUIRE(VectorNear(vec4(0.0, 0.0, 0.3827, 0.9239), qz.get_as_vec(), precision));
    // MATLAB: axang2quat([0.5 1.0 -1.0 23*pi/180])
    REQUIRE(VectorNear(vec4(0.0665, 0.1329, -0.1329, 0.9799), qxyz.get_as_vec(), precision));
}

TEST_CASE("Copy ctor is used.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(q1);
    REQUIRE(vec4(1.0, 2.0, 3.0, 4.0) == q2.get_as_vec());
}

TEST_CASE("Move ctor is used.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    REQUIRE(vec4(1.0, 2.0, 3.0, 4.0) == q1.get_as_vec());
}

TEST_CASE("Quaternion identity element.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    q1.normalize();
    quat q2;
    q2.set_identity();

    quat q12(q1*q2);
    quat q21(q2*q1);

    REQUIRE(VectorNear(q1.get_as_vec(), q12.get_as_vec(), precision));
    REQUIRE(VectorNear(q1.get_as_vec(), q21.get_as_vec(), precision));
}

TEST_CASE("A quat is initialized then normalized.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    q1.normalize();
    REQUIRE(VectorNear(vec4(0.1826, 0.3651, 0.5477, 0.7303), q1.get_as_vec(), precision));
}

TEST_CASE("A quat is initialized and then conjugated.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    q1.conjugate();
    REQUIRE(vec4(-1.0, -2.0, -3.0, 4.0) == q1.get_as_vec());
}

TEST_CASE("A quat is assigned the conjugate of another one.", "[quat]")
{
    quat q1(0.2, -0.5, -3.1, 4.25);
    quat q1c = q1.get_conjugate();
    REQUIRE(vec4(0.2, -0.5, -3.1, 4.25) == q1.get_as_vec());
    REQUIRE(vec4(-0.2, 0.5, 3.1, 4.25) == q1c.get_as_vec());
}

TEST_CASE("Two quats are inversed.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.2, -0.5, -3.1, 4.25);
    quat q1inv = q1.get_inverse();
    quat q2inv = q2.get_inverse();
    // MATLAB: quatinv([4 1 2 3])
    REQUIRE(VectorNear(vec4(-0.0333, -0.0667, -0.1000, 0.1333), q1inv.get_as_vec(), precision));
    // MATLAB: quatinv([4.25 0.2 -0.5 -3.1])
    REQUIRE(VectorNear(vec4(-0.0072, 0.0179, 0.1109, 0.1520), q2inv.get_as_vec(), precision));
}

TEST_CASE("Getting Euler angles back from a quat.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.75, -1.25, 3.21, 2.1);
    vec3 eul1 = q1.get_euler_angles(false);
    vec3 eul2 = q2.get_euler_angles(false);
    // MATLAB: quat2eul([4 1 2 3])
    REQUIRE(VectorNear(vec3(0.7854, 0.3398, 1.4289), eul1, precision));
    // MATLAB: quat2eul([2.1 0.75 -1.25 3.21])
    REQUIRE(VectorNear(vec3(-0.3695, -0.6406, 2.1068), eul2, precision));
}

TEST_CASE("Getting axis and angle back from a quat.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.7071, 0.0, 0.0, 0.7071);
    vec4 axang1 = q1.get_axis_angle(false);
    vec4 axang2 = q2.get_axis_angle(false);
    // MATLAB: quat2axang([4.0 1.0 2.0 3.0])
    REQUIRE(VectorNear(vec4(0.2673, 0.5345, 0.8018, 1.5041), axang1, precision));
    // MATLAB: quat2axang([0.7071 0.7071 0 0])
    REQUIRE(VectorNear(vec4(1.0000, 0, 0, 1.5708), axang2, precision));
}

TEST_CASE("Forming a rotation matrix from a quat.", "[quat]")
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.75, -1.25, 3.21, 2.1);
    mat4 m1 = q1.get_rotation_matrix();
    mat4 m2 = q2.get_rotation_matrix();
    // MATLAB: quat2rotm([4.0 1.0 2.0 3.0])
    mat4 m1expected(0.1333,  -0.6667, 0.7333, 0.0,
                    0.9333,  0.3333,  0.1333, 0.0,
                    -0.3333, 0.6667,  0.6667, 0.0,
                    0.0,     0.0,     0.0,    1.0);
    // MATLAB: quat2rotm([2.1 0.75 -1.25 3.21])
    mat4 m2expected(-0.4094, -0.9120, -0.0258, 0.0,
                    0.6893,  -0.2906, -0.6636, 0.0,
                    0.5977,  -0.2895, 0.7476,  0.0,
                    0.0,     0.0,     0.0,     1.0);
    REQUIRE(MatrixNear(m1expected, m1, precision));
    REQUIRE(MatrixNear(m2expected, m2, precision));
}

TEST_CASE("Forming a rotation matrix from Euler initialized quats.", "[quat]")
{
    SECTION("Basis: x-axis 60° rotation.")
    {
        quat qx(0.0, 0.0, 60.0);
        mat4 mx = qx.get_rotation_matrix();
        mat4 expect(1.0, 0.0, 0.0, 0.0,
                    0.0, 0.5, -0.866, 0.0,
                    0.0, 0.866, 0.5, 0.0,
                    0.0, 0.0, 0.0, 1.0);
        REQUIRE(MatrixNear(expect, mx, precision));
    }

    SECTION("Basis: y-axis 60° rotation.")
    {
        quat qy(0.0, 60.0, 0.0);
        mat4 my = qy.get_rotation_matrix();
        mat4 expect(0.5, 0.0, 0.866, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    -0.866, 0.0, 0.5, 0.0,
                    0.0, 0.0, 0.0, 1.0);
        REQUIRE(MatrixNear(expect, my, precision));
    }

    SECTION("Basis: z-axis 60° rotation.")
    {
        quat qz(60.0, 0.0, 0.0);
        mat4 mz = qz.get_rotation_matrix();
        mat4 expect(0.5, -0.866, 0.0, 0.0,
                    0.866, 0.5, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0);
        REQUIRE(MatrixNear(expect, mz, precision));
    }
}

TEST_CASE("Two quats are multiplied.", "[quat]")
{
    quat q1(0.7071, 0.0, 0.0, 0.7071);
    quat q2(0.0, 0.7071, 0.0, 0.7071);
    quat q12 = q1*q2;
    quat q21 = q2*q1;

    //MATLAB: quatmultiply([0.7071 0.7071 0 0],[0.7071 0 0.7071 0])
    REQUIRE(VectorNear(vec4(0.5000, 0.5000, 0.5000, 0.5000), q12.get_as_vec(), precision));
    //MATLAB: quatmultiply([0.7071 0 0.7071 0], [0.7071 0.7071 0 0])
    REQUIRE(VectorNear(vec4(0.5000, 0.5000, -0.5000, 0.5000), q21.get_as_vec(), precision));
}

TEST_CASE("Two quats are divided.", "[quat]")
{
    quat q1(0.7071, 0.0, 0.0, 0.7071);
    quat q2(0.0, 0.7071, 0.0, 0.7071);
    quat q12 = q1/q2;
    quat q21 = q2/q1;

    //MATLAB: quatmultiply([0.7071 0.7071 0 0],[0.7071 0 0.7071 0])
    REQUIRE(VectorNear(vec4(0.5000, -0.5000, 0.5000, 0.5000), q12.get_as_vec(), precision));
    //MATLAB: quatmultiply([0.7071 0 0.7071 0], [0.7071 0.7071 0 0])
    REQUIRE(VectorNear(vec4(-0.5000, 0.5000, -0.5000, 0.5000), q21.get_as_vec(), precision));
}

TEST_CASE("A quat acts on a vector to rotate it.", "[quat]")
{
    quat q1(0.7071, 0.0, 0.0, 0.7071);
    quat q2(0.0, 0.7071, 0.0, 0.7071);
    quat q3(0.75, -1.25, 3.21, 2.1);
    vec3 v1(1.0, 1.0, 1.0);
    vec3 v2(-0.5, 0.2, 1.8);
    vec3 v11(q1.rotate(v1));
    vec3 v21(q2.rotate(v1));
    vec3 v31(q3.rotate(v1));
    vec3 v32(q3.rotate(v2));
    //MATLAB: quatrotate([0.7071 0.7071 0 0],[1 1 1])
    REQUIRE(VectorNear(vec3(1.0000, 1.0000, -1.0000), v11, precision));
    //MATLAB: quatrotate([0.7071 0 0.7071 0],[1 1 1])
    REQUIRE(VectorNear(vec3(-1.0000, 1.0000, 1.0000), v21, precision));
    //MATLAB: quatrotate([2.1 0.75 -1.25 3.21],[1 1 1])
    REQUIRE(VectorNear(vec3(0.8776, -1.4921, 0.0581), v31, precision));
    //MATLAB: quatrotate([2.1 0.75 -1.25 3.21],[-0.5 0.2 1.8])
    REQUIRE(VectorNear(vec3(1.4185, -0.1232, 1.2259), v32, precision));
}
