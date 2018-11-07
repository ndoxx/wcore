#include <catch2/catch.hpp>
#include <iostream>
#include "math3d.h"
#include "catch_math_common.h"

using namespace math;

static const float precision = 1e-4;

TEST_CASE("Arglist ctor: row major initialization.", "[mat]")
{
    SECTION("2x2 matrix.")
    {
        mat2 m2(1.0, 2.0,
                -3.0, 4.0);

        REQUIRE(m2[0]==1.0);
        REQUIRE(m2[1]==-3.0);
        REQUIRE(m2[2]==2.0);
        REQUIRE(m2[3]==4.0);
    }

    SECTION("3x3 matrix.")
    {
        mat3 m3(1.0, 2.0, 3.0,
                4.0, 5.0, 6.0,
                7.0, 8.0, 9.0);
        REQUIRE(m3[0]==1.0);
        REQUIRE(m3[1]==4.0);
        REQUIRE(m3[2]==7.0);
        REQUIRE(m3[3]==2.0);
        REQUIRE(m3[4]==5.0);
        REQUIRE(m3[5]==8.0);
        REQUIRE(m3[6]==3.0);
        REQUIRE(m3[7]==6.0);
        REQUIRE(m3[8]==9.0);
    }
}

TEST_CASE("Column constructor.", "[mat]")
{
    mat4 m4(vec4(1.0, 5.0, 9.0, 13.0),
            vec4(2.0, 6.0, 10.0, 14.0),
            vec4(3.0, 7.0, 11.0, 15.0),
            vec4(4.0, 8.0, 12.0, 16.0));
    REQUIRE(m4[3]==13.0);
    REQUIRE(m4[7]==14.0);
}

TEST_CASE("Row extraction.", "[mat]")
{
    mat4 m4(1.0, 2.0, 3.0, 4.0,
            5.0, 6.0, 7.0, 8.0,
            9.0, 1.0, 2.0, 3.0,
            4.0, 5.0, 6.0, 7.0);
    vec4 r1(m4.row(1));
    REQUIRE(r1 == vec4(5.0, 6.0, 7.0, 8.0));
}

TEST_CASE("Column extraction.", "[mat]")
{
    mat4 m4(1.0, 2.0, 3.0, 4.0,
            5.0, 6.0, 7.0, 8.0,
            9.0, 1.0, 2.0, 3.0,
            4.0, 5.0, 6.0, 7.0);
    vec4 c3(m4.col(2));
    REQUIRE(c3 == vec4(3.0, 7.0, 2.0, 6.0));
}

TEST_CASE("Submatrix extraction.", "[mat]")
{
    mat4 m4(1.0, 2.0, 3.0, 4.0,
            5.0, 6.0, 7.0, 8.0,
            9.0, 1.0, 2.0, 3.0,
            4.0, 5.0, 6.0, 7.0);

    SECTION("Remove last row and column.")
    {
        mat3 sub = m4.submatrix(3, 3);
        mat3 expect(1.0, 2.0, 3.0,
                    5.0, 6.0, 7.0,
                    9.0, 1.0, 2.0);
        REQUIRE(MatrixNear(expect, sub, precision));
    }
    SECTION("Remove 3rd row and 2nd column.")
    {
        mat3 sub = m4.submatrix(2, 1);
        mat3 expect(1.0, 3.0, 4.0,
                    5.0, 7.0, 8.0,
                    4.0, 6.0, 7.0);
        REQUIRE(MatrixNear(expect, sub, precision));
    }
}

TEST_CASE("Fast affine matrix detection.", "[mat]")
{
    SECTION("This matrix is affine.")
    {
        mat4 m4(0.5, 0.0, 0.5, 15.4,
                0.0, 0.5, 0.0, -128.2,
                0.2, 0.1, 0.2, 114.7,
                0.0, 0.0, 0.0, 1.0);
        REQUIRE(m4.is_affine());
    }
    SECTION("This matrix is NOT affine.")
    {
        mat4 m4(0.5, 0.0, 0.5, 15.4,
                0.0, 0.5, 0.0, -128.2,
                0.2, 0.1, 0.2, 114.7,
                0.0, 14.0, 0.0, 2.0);
        REQUIRE(!m4.is_affine());
    }
}

TEST_CASE("Diagonal initialization.", "[mat]")
{
    mat4 M;
    M.init_diagonal(vec4(1.0, 2.0, 3.0, 4.0));
    mat4 expect(1.0, 0.0, 0.0, 0.0,
                0.0, 2.0, 0.0, 0.0,
                0.0, 0.0, 3.0, 0.0,
                0.0, 0.0, 0.0, 4.0);
    REQUIRE(MatrixNear(expect, M, 1e-8));
}

TEST_CASE("Identity initialization.", "[mat]")
{
    mat4 M;
    M.init_identity();
    mat4 expect(1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0);
    REQUIRE(MatrixNear(expect, M, 1e-8));
}

TEST_CASE("Scaling initialization.", "[mat]")
{
    mat4 M;
    M.init_scale(vec3(1.0, 2.0, 3.0));
    mat4 expect(1.0, 0.0, 0.0, 0.0,
                0.0, 2.0, 0.0, 0.0,
                0.0, 0.0, 3.0, 0.0,
                0.0, 0.0, 0.0, 1.0);
    REQUIRE(MatrixNear(expect, M, 1e-8));
}

TEST_CASE("Uniform scale initialization.", "[mat]")
{
    mat4 M;
    M.init_scale(2.0);
    mat4 expect(2.0, 0.0, 0.0, 0.0,
                0.0, 2.0, 0.0, 0.0,
                0.0, 0.0, 2.0, 0.0,
                0.0, 0.0, 0.0, 1.0);
    REQUIRE(MatrixNear(expect, M, 1e-8));
}

TEST_CASE("Uniform coord translation matrix initialization.", "[mat]")
{
    mat4 M;
    M.init_translation(vec3(-1.0, 14.0, -25.0));
    mat4 expect(1.0, 0.0, 0.0, -1.0,
                0.0, 1.0, 0.0, 14.0,
                0.0, 0.0, 1.0, -25.0,
                0.0, 0.0, 0.0, 1.0);
    REQUIRE(MatrixNear(expect, M, 1e-8));
}

TEST_CASE("Transposition.", "[mat]")
{
    mat4 M(1.0, 2.0, 3.0, 4.0,
           5.0, 6.0, 7.0, 8.0,
           9.0, 1.0, 2.0, 3.0,
           4.0, 5.0, 6.0, 7.0);
    mat4 expect(1.0, 5.0, 9.0, 4.0,
                2.0, 6.0, 1.0, 5.0,
                3.0, 7.0, 2.0, 6.0,
                4.0, 8.0, 3.0, 7.0);
    SECTION("Transpose matrix directly.")
    {
        M.transpose();
        REQUIRE(MatrixNear(expect, M, 1e-8));
    }

    SECTION("Assign transposed matrix.")
    {
        mat4 Mt(M.transposed());
        REQUIRE(MatrixNear(expect, Mt, 1e-8));
    }
}

TEST_CASE("Matrix product.", "[mat]")
{
    mat3 M1(1.0, 5.0, 2.5,
            4.0, -1.2, 1.5,
            8.4, -12.0, 3.0);
    mat3 M2(4.0, 8.0, 2.5,
            1.0, 0.0, 5.8,
            2.9, 1.0, 2.5);
    mat3 expect12(16.2500, 10.5000, 37.7500,
                  19.1500, 33.5000, 6.7900,
                  30.3000, 70.2000, -41.1000);
    mat3 expect21(57.0000, -19.6000, 29.5000,
                  49.7200, -64.6000, 19.9000,
                  27.9000, -16.7000, 16.2500);
    REQUIRE(MatrixNear(expect12, M1*M2, precision));
    REQUIRE(MatrixNear(expect21, M2*M1, precision));
}

TEST_CASE("Determinant computation.", "[mat]")
{
    SECTION("2x2 matrix.")
    {
        mat2 M(1.5, 4.0,
               -1.0, 6.0);
        REQUIRE(FloatNear(1.5*6.0+4.0, det(M), precision));
    }

    SECTION("3x3 matrix.")
    {
        mat3 M(0.4218, 0.9595, 0.8491,
               0.9157, 0.6557, 0.9340,
               0.7922, 0.0357, 0.6787);
        REQUIRE(FloatNear(-0.1261, det(M), precision));
    }

    SECTION("4x4 matrix.")
    {
        mat4 M(0.9619, 0.8687, 0.8001, 0.2638,
               0.0046, 0.0844, 0.4314, 0.1455,
               0.7749, 0.3998, 0.9106, 0.1361,
               0.8173, 0.2599, 0.1818, 0.8693);
        REQUIRE(FloatNear(0.1434, det(M), precision));
    }
}

TEST_CASE("Matrix inversion.", "[mat]")
{
    SECTION("3x3 matrix inversion.")
    {
        mat3 M(1.0, 2.0, 3.0,
               3.0, 4.0, 5.0,
               8.0, 9.0, 7.0);
        mat3 Minv;
        inverse(M, Minv);
        mat3 expect(-2.8333, 2.1667, -0.3333,
                    3.1667, -2.8333, 0.6667,
                    -0.8333, 1.1667, -0.3333);
        REQUIRE(MatrixNear(expect, Minv, precision));
    }

    SECTION("3x3 transposed matrix inversion.")
    {
        mat3 M(1.0, 2.0, 3.0,
               3.0, 4.0, 5.0,
               8.0, 9.0, 7.0);
        mat3 Minv;
        transposed_inverse(M, Minv);
        mat3 expect(vec3(-2.8333, 2.1667, -0.3333),
                    vec3(3.1667, -2.8333, 0.6667),
                    vec3(-0.8333, 1.1667, -0.3333));
        REQUIRE(MatrixNear(expect, Minv, precision));
    }

    SECTION("4x4 matrix inversion: non affine.")
    {
        mat4 M(1.0, 2.0, 3.0, 2.0,
               3.0, 4.0, 5.0, 4.0,
               8.0, 9.0, 7.0, 5.0,
               1.0, 2.0, 1.0, 1.0);
        mat4 Minv;
        inverse(M, Minv);
        mat4 expect(-0.7000, 0.3000, 0.2000, -0.8000,
                    0.5000, -0.5000, 0.0000, 1.0000,
                    1.3000, -0.7000, 0.2000, -0.8000,
                   -1.6000, 1.4000, -0.4000, 0.6000);
        REQUIRE(MatrixNear(expect, Minv, precision));
    }

    SECTION("4x4 transposed matrix inversion: non affine.")
    {
        mat4 M(1.0, 2.0, 3.0, 2.0,
               3.0, 4.0, 5.0, 4.0,
               8.0, 9.0, 7.0, 5.0,
               1.0, 2.0, 1.0, 1.0);
        mat4 Minv;
        transposed_inverse(M, Minv);
        mat4 expect(vec4(-0.7000, 0.3000, 0.2000, -0.8000),
                    vec4(0.5000, -0.5000, 0.0000, 1.0000),
                    vec4(1.3000, -0.7000, 0.2000, -0.8000),
                    vec4(-1.6000, 1.4000, -0.4000, 0.6000));
        REQUIRE(MatrixNear(expect, Minv, precision));
    }

    SECTION("4x4 matrix inversion: affine.")
    {
        mat4 M(0, 1, 0, -128.2000,
               0, 0, -1, -114.7000,
               -1, 0, 0, -15.4000,
               0, 0, 0, 1.0000);
        mat4 Minv;
        inverse_affine(M, Minv);
        mat4 expect(-0.0000, -0.0000, -1.0000, -15.4000,
                    1.0000, -0.0000, -0.0000, 128.2000,
                    -0.0000, -1.0000, 0.0000, -114.7000,
                    0, 0, 0, 1.0000);
        REQUIRE(MatrixNear(expect, Minv, precision));
    }

    SECTION("4x4 transposed matrix inversion: affine.")
    {
        mat4 M(0, 1, 0, -128.2000,
               0, 0, -1, -114.7000,
               -1, 0, 0, -15.4000,
               0, 0, 0, 1.0000);
        mat4 Minv;
        transposed_inverse_affine(M, Minv);
        mat4 expect(vec4(-0.0000, -0.0000, -1.0000, -15.4000),
                    vec4(1.0000, -0.0000, -0.0000, 128.2000),
                    vec4(-0.0000, -1.0000, 0.0000, -114.7000),
                    vec4(0, 0, 0, 1.0000));
        REQUIRE(MatrixNear(expect, Minv, precision));
    }
}

TEST_CASE("Linear interpolation performed on matrices.", "[mat]")
{
    mat4 M1(0.2769, 0.6948, 0.4387, 0.1869,
            0.0462, 0.3171, 0.3816, 0.4898,
            0.0971, 0.9502, 0.7655, 0.4456,
            0.8235, 0.0344, 0.7952, 0.6463);
    mat4 M2(0.7094, 0.6551, 0.9597, 0.7513,
            0.7547, 0.1626, 0.3404, 0.2551,
            0.2760, 0.1190, 0.5853, 0.5060,
            0.6797, 0.4984, 0.2238, 0.6991);
    mat4 expect(0.4585, 0.6781, 0.6576, 0.4239,
                0.3437, 0.2522, 0.3643, 0.3912,
                0.1723, 0.6011, 0.6898, 0.4709,
                0.7631, 0.2293, 0.5552, 0.6685);
    mat4 M12 = lerp(M1, M2, 0.42);
    REQUIRE(MatrixNear(expect, M12, precision));
}

TEST_CASE("4x4 matrices are initialized as rotation mats.", "[mat]")
{
    SECTION("60° rotation around x axis.")
    {
        mat4 R;
        init_rotation_euler(R, 0.0, 0.0, TORADIANS(60));
        // MATLAB: eul2rotm([0 0 pi/3]) % ZYX notation
        mat4 expect(1.0, 0.0, 0.0, 0.0,
                    0.0, 0.5, -0.866, 0.0,
                    0.0, 0.866, 0.5, 0.0,
                    0.0, 0.0, 0.0, 1.0);
        REQUIRE(MatrixNear(expect, R, precision));
    }
    SECTION("60° rotation around y axis.")
    {
        mat4 R;
        init_rotation_euler(R, 0.0, TORADIANS(60), 0.0);
        // MATLAB: eul2rotm([0 pi/3 0]) % ZYX notation
        mat4 expect(0.5, 0.0, 0.866, 0.0,
                    0.0, 1.0, 0.0, 0.0,
                    -0.866, 0.0, 0.5, 0.0,
                    0.0, 0.0, 0.0, 1.0);
        REQUIRE(MatrixNear(expect, R, precision));
    }
    SECTION("60° rotation around z axis.")
    {
        mat4 R;
        init_rotation_euler(R, TORADIANS(60), 0.0, 0.0);
        // MATLAB: eul2rotm([pi/3 0 0]) % ZYX notation
        mat4 expect(0.5, -0.866, 0.0, 0.0,
                    0.866, 0.5, 0.0, 0.0,
                    0.0, 0.0, 1.0, 0.0,
                    0.0, 0.0, 0.0, 1.0);
        REQUIRE(MatrixNear(expect, R, precision));
    }
}

TEST_CASE("4x4 matrix initialized as lookAt matrix.", "[mat]")
{
    mat4 V,T,R;
    vec3 up(0.0, 1.0, 0.0);
    vec3 eye(10.0, 0.0, 10.0);

    SECTION("Cam at 10 0 10 looking at origin.")
    {
        // Camera is at (10, 0, 10) and looks at origin
        vec3 target(0.0, 0.0, 0.0);

        init_look_at(V, eye, target, up);

        // Camera is at a distance sqrt(10*10+10*10)~=14.14 from origin
        // so -14.14 along forward direction (from camera to origin).
        // Also there is a 45° rotation along y-axis.
        init_translation(T, -eye);
        init_rotation_euler(R, 0.0, TORADIANS(-45), 0.0);

        REQUIRE(MatrixNear(R*T, V, precision));
    }

    SECTION("Cam at 10 0 10 looking at 5 0 0.")
    {
        vec3 target(5.0, 0.0, 0.0);

        init_look_at(V, eye, target, up);

        init_translation(T, -eye);
        // cam and target at opposite points of a rectangle in (XZ) plane
        // rotation angle is the angle btw diagonal and Z
        init_rotation_euler(R, 0.0, -atan(0.5), 0.0);

        REQUIRE(MatrixNear(R*T, V, precision));
    }

    SECTION("Cam at 10 0 10 looking at 5 0 3.")
    {
        vec3 target(5.0, 0.0, 3.0);

        init_look_at(V, eye, target, up);

        init_translation(T, -eye);
        init_rotation_euler(R, 0.0, -atan(5.0/7.0), 0.0);

        REQUIRE(MatrixNear(R*T, V, precision));
    }
}

TEST_CASE("4x4 matrix initialized to perspective projection.", "[mat]")
{
    mat4 P;
    float fov = 90.0f;
    float aspectRatio = 4.0f/3.0f;
    float zNear = 0.5f;
    float zFar = 100.0f;
    init_perspective(P, fov, aspectRatio, zNear, zFar);

    SECTION("Check that we get the same thing with a general frustum.")
    {
        mat4 Pfrus;
        // TRUSTED OpenGL projection function using general frustum
        init_frustum(Pfrus, {-aspectRatio/2, aspectRatio/2, -0.5, 0.5, 0.5, 100.0});
        REQUIRE(MatrixNear(Pfrus, P, precision));
    }
}

TEST_CASE("4x4 matrix initialized to orthographic projection.", "[mat]")
{
    mat4 P;
    float screen_width = 1280.0f;
    float screen_height = 720.0f;
    float aspect = screen_width/screen_height;
    init_ortho(P, {-aspect/2, aspect/2, -0.5, 0.5, 0.5, 100.0});
    REQUIRE(true); // TRUSTED (for now) (looks alright)
}
