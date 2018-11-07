#include <gtest/gtest.h>
#include <tuple>
#include "math3d.h"
#include "quaternion.h"

/*
    DEPREC
    Old quaternion test program using gTest.
    Unit test now done using Catch 2.
*/

using namespace math;

static const float precision = 1e-4;

::testing::AssertionResult VectorNear(const vec3& expected, const vec3& result, float delta)
{
    vec3 err(expected-result);
    bool less_than_delta = true;
    for (unsigned ii = 0; ii < 3; ++ii)
        less_than_delta &= (err[ii]<delta);

    if (less_than_delta)
        return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << "Vectors differ by more than " << delta
           << std::endl << "EXPECTED: " << expected << " GOT: " << result;
}

::testing::AssertionResult VectorNear(const vec4& expected, const vec4& result, float delta)
{
    vec4 err(expected-result);
    bool less_than_delta = true;
    for (unsigned ii = 0; ii < 4; ++ii)
        less_than_delta &= (err[ii]<delta);

    if (less_than_delta)
        return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << "Vectors differ by more than " << delta
           << std::endl << "EXPECTED: " << expected << " GOT: " << result;
}

::testing::AssertionResult MatrixNear(const mat4& expected, const mat4& result, float delta)
{
    mat4 err(expected-result);
    bool less_than_delta = true;
    for (unsigned ii = 0; ii < 16; ++ii)
        less_than_delta &= (err[ii]<delta);

    if (less_than_delta)
        return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << "Matrices differ by more than " << delta
           << std::endl << "EXPECTED: " << std::endl << expected
           << " GOT: " << std::endl << result;
}
/*
class QuatTest: testing::Test
{
protected:
    void SetUp()
    {
        q = new quat;
    }
    void TearDown()
    {
        delete q; q = nullptr;
    }

    quat* q;
};

using TestParamInterface = std::tuple<float, float, float, vec4>;
class QuatEulerTest: QuatTest, testing::WithParamInterface<TestParamInterface>
{
protected:
    void SetUp() override
    {
        auto as = GetParam();
        float phi   = std::get<0>(as);
        float theta = std::get<1>(as);
        float psi   = std::get<2>(as);

        q = new quat(phi, theta, psi);
        auto value = q->get_as_vec();
    }
};

TEST_P(QuatEulerTest, InitEulerAngles)
{
    auto as = GetParam();
    auto expect = std::get<3>(as);
    EXPECT_TRUE(VectorNear(expect, value, precision));
}

INSTANTIATE_TEST_CASE_P(
    testing::Default, QuatEulerTest,testing::Values(
        quatState{30.0, 0.0, 0.0,    vec4(0.0, 0.0, 0.2588, 0.9659)},
        quatState{0.0, 30.0, 0.0,    vec4(0.0, 0.2588, 0.0, 0.9659)},
        quatState{0.0, 0.0, 30.0,    vec4(0.2588, 0.0, 0.0, 0.9659)},
        quatState{30.0, -25.2, 42.5, vec4(0.3943, -0.1048, 0.3118, 0.8581)}
    )
);*/

TEST(QuaternionTest, DefaultInit)
{
    quat qq;
    EXPECT_EQ(vec4(0.0, 0.0, 0.0, 1.0), qq.get_as_vec());
}

TEST(QuaternionTest, ComponentInit)
{
    quat qq(1.0, 2.0, 3.0, 4.0);
    EXPECT_EQ(vec4(1.0, 2.0, 3.0, 4.0), qq.get_as_vec());
}

TEST(QuaternionTest, EulerInit)
{
    quat qx(30.0, 0.0, 0.0);
    quat qy(0.0, 30.0, 0.0);
    quat qz(0.0, 0.0, 30.0);
    quat qxyz(30.0, -25.2, 42.5);
    // MATLAB: eul2quat([pi*30/180, 0, 0])
    EXPECT_TRUE(VectorNear(vec4(0.0, 0.0, 0.2588, 0.9659), qx.get_as_vec(), precision));
    // MATLAB: eul2quat([0, pi*30/180, 0])
    EXPECT_TRUE(VectorNear(vec4(0.0, 0.2588, 0.0, 0.9659), qy.get_as_vec(), precision));
    // MATLAB: eul2quat([0, 0, pi*30/180])
    EXPECT_TRUE(VectorNear(vec4(0.2588, 0.0, 0.0, 0.9659), qz.get_as_vec(), precision));
    // MATLAB: eul2quat([30.0 -25.2 42.5]*(pi/180.0))
    EXPECT_TRUE(VectorNear(vec4(0.3943, -0.1048, 0.3118, 0.8581), qxyz.get_as_vec(), precision));
}

TEST(QuaternionTest, AxisAngleInit)
{
    quat qx(vec3(1.0, 0.0, 0.0), 45.0);
    quat qy(vec3(0.0, 1.0, 0.0), 45.0);
    quat qz(vec3(0.0, 0.0, 1.0), 45.0);
    quat qxyz(vec3(0.5, 1.0, -1.0), 23.0);
    // MATLAB: axang2quat([1 0 0 pi/4])
    EXPECT_TRUE(VectorNear(vec4(0.3827, 0.0, 0.0, 0.9239), qx.get_as_vec(), precision));
    // MATLAB: axang2quat([0 1 0 pi/4])
    EXPECT_TRUE(VectorNear(vec4(0.0, 0.3827, 0.0, 0.9239), qy.get_as_vec(), precision));
    // MATLAB: axang2quat([0 0 1 pi/4])
    EXPECT_TRUE(VectorNear(vec4(0.0, 0.0, 0.3827, 0.9239), qz.get_as_vec(), precision));
    // MATLAB: axang2quat([0.5 1.0 -1.0 23*pi/180])
    EXPECT_TRUE(VectorNear(vec4(0.0665, 0.1329, -0.1329, 0.9799), qxyz.get_as_vec(), precision));
}

TEST(QuaternionTest, CopyCtor)
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(q1);
    EXPECT_EQ(vec4(1.0, 2.0, 3.0, 4.0), q2.get_as_vec());
}

TEST(QuaternionTest, MoveCtor)
{
    quat q1(quat(1.0, 2.0, 3.0, 4.0));
    EXPECT_EQ(vec4(1.0, 2.0, 3.0, 4.0), q1.get_as_vec());
}

TEST(QuaternionTest, Normalize)
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    q1.normalize();
    EXPECT_TRUE(VectorNear(vec4(0.1826, 0.3651, 0.5477, 0.7303), q1.get_as_vec(), precision));
}

TEST(QuaternionTest, Conjugate)
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.2, -0.5, -3.1, 4.25);
    q1.conjugate();
    quat q2c = q2.get_conjugate();
    EXPECT_EQ(vec4(-1.0, -2.0, -3.0, 4.0), q1.get_as_vec());
    EXPECT_EQ(vec4(0.2, -0.5, -3.1, 4.25), q2.get_as_vec());
    EXPECT_EQ(vec4(-0.2, 0.5, 3.1, 4.25), q2c.get_as_vec());
}

TEST(QuaternionTest, Inverse)
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.2, -0.5, -3.1, 4.25);
    quat q1inv = q1.get_inverse();
    quat q2inv = q2.get_inverse();
    // MATLAB: quatinv([4 1 2 3])
    EXPECT_TRUE(VectorNear(vec4(-0.0333, -0.0667, -0.1000, 0.1333), q1inv.get_as_vec(), precision));
    // MATLAB: quatinv([4.25 0.2 -0.5 -3.1])
    EXPECT_TRUE(VectorNear(vec4(-0.0072, 0.0179, 0.1109, 0.1520), q2inv.get_as_vec(), precision));
}

TEST(QuaternionTest, GetEulerAngles)
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.75, -1.25, 3.21, 2.1);
    vec3 eul1 = q1.get_euler_angles(false);
    vec3 eul2 = q2.get_euler_angles(false);
    // MATLAB: quat2eul([4 1 2 3])
    EXPECT_TRUE(VectorNear(vec3(0.7854, 0.3398, 1.4289), eul1, precision));
    // MATLAB: quat2eul([2.1 0.75 -1.25 3.21])
    EXPECT_TRUE(VectorNear(vec3(-0.3695, -0.6406, 2.1068), eul2, precision));
}

TEST(QuaternionTest, GetAxisAngle)
{
    quat q1(1.0, 2.0, 3.0, 4.0);
    quat q2(0.7071, 0.0, 0.0, 0.7071);
    vec4 axang1 = q1.get_axis_angle(false);
    vec4 axang2 = q2.get_axis_angle(false);
    // MATLAB: quat2axang([4.0 1.0 2.0 3.0])
    EXPECT_TRUE(VectorNear(vec4(0.2673, 0.5345, 0.8018, 1.5041), axang1, precision));
    // MATLAB: quat2axang([0.7071 0.7071 0 0])
    EXPECT_TRUE(VectorNear(vec4(1.0000, 0, 0, 1.5708), axang2, precision));
}

TEST(QuaternionTest, GetRotationMatrix)
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
    EXPECT_TRUE(MatrixNear(m1expected, m1, precision));
    EXPECT_TRUE(MatrixNear(m2expected, m2, precision));
}

TEST(QuaternionTest, Multiplication)
{
    quat q1(0.7071, 0.0, 0.0, 0.7071);
    quat q2(0.0, 0.7071, 0.0, 0.7071);
    quat q12 = q1*q2;
    quat q21 = q2*q1;

    //MATLAB: quatmultiply([0.7071 0.7071 0 0],[0.7071 0 0.7071 0])
    EXPECT_TRUE(VectorNear(vec4(0.5000, 0.5000, 0.5000, 0.5000), q12.get_as_vec(), precision));
    //MATLAB: quatmultiply([0.7071 0 0.7071 0], [0.7071 0.7071 0 0])
    EXPECT_TRUE(VectorNear(vec4(0.5000, 0.5000, 0.5000, -0.5000), q21.get_as_vec(), precision));
}

TEST(QuaternionTest, Division)
{
    quat q1(0.7071, 0.0, 0.0, 0.7071);
    quat q2(0.0, 0.7071, 0.0, 0.7071);
    quat q12 = q1/q2;
    quat q21 = q2/q1;

    //MATLAB: quatmultiply([0.7071 0.7071 0 0],[0.7071 0 0.7071 0])
    EXPECT_TRUE(VectorNear(vec4(0.5000, -0.5000, 0.5000, 0.5000), q12.get_as_vec(), precision));
    //MATLAB: quatmultiply([0.7071 0 0.7071 0], [0.7071 0.7071 0 0])
    EXPECT_TRUE(VectorNear(vec4(-0.5000, 0.5000, -0.5000, 0.5000), q21.get_as_vec(), precision));
}

TEST(QuaternionTest, RotateVector)
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
    EXPECT_TRUE(VectorNear(vec3(1.0000, 1.0000, -1.0000), v11, precision));
    //MATLAB: quatrotate([0.7071 0 0.7071 0],[1 1 1])
    EXPECT_TRUE(VectorNear(vec3(-1.0000, 1.0000, 1.0000), v21, precision));
    //MATLAB: quatrotate([2.1 0.75 -1.25 3.21],[1 1 1])
    EXPECT_TRUE(VectorNear(vec3(0.8776, -1.4921, 0.0581), v31, precision));
    //MATLAB: quatrotate([2.1 0.75 -1.25 3.21],[-0.5 0.2 1.8])
    EXPECT_TRUE(VectorNear(vec3(1.4185, -0.1232, 1.2259), v32, precision));
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
