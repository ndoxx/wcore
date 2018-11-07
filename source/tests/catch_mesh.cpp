#include <catch2/catch.hpp>
#include <iostream>

#define __DEBUG__

#include "mesh.hpp"
#include "catch_math_common.h"

using namespace math;

static float precision = 1e-6;

static void init_cube_3P3N(Mesh<Vertex3P3N>& mesh)
{
    mesh.emplace_vertex(vec3( 0.5f, 0.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 1.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 1.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 0.0f, 0.5f), vec3(0,0,0));

    //Right
    mesh.emplace_vertex(vec3( 0.5f, 0.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 1.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 1.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 0.0f, 0.5f), vec3(0,0,0));

    //Back
    mesh.emplace_vertex(vec3(-0.5f, 0.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 1.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 1.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 0.0f,-0.5f), vec3(0,0,0));

    //Left
    mesh.emplace_vertex(vec3(-0.5f, 0.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 1.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 1.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 0.0f,-0.5f), vec3(0,0,0));

    //Top
    mesh.emplace_vertex(vec3( 0.5f, 1.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 1.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 1.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 1.0f, 0.5f), vec3(0,0,0));

    //Bottom
    mesh.emplace_vertex(vec3( 0.5f, 0.0f,-0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 0.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 0.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 0.0f,-0.5f), vec3(0,0,0));

    mesh.push_triangle(0,  1,  2);
    mesh.push_triangle(0,  2,  3);
    mesh.push_triangle(4,  5,  6);
    mesh.push_triangle(4,  6,  7);
    mesh.push_triangle(8,  9,  10);
    mesh.push_triangle(8,  10, 11);
    mesh.push_triangle(12, 13, 14);
    mesh.push_triangle(12, 14, 15);
    mesh.push_triangle(16, 17, 18);
    mesh.push_triangle(16, 18, 19);
    mesh.push_triangle(20, 21, 22);
    mesh.push_triangle(20, 22, 23);
    mesh.build_normals();
}

TEST_CASE("Mesh with a single triangle.", "[mesh]")
{
    Mesh<Vertex3P3N> mesh;
    mesh.emplace_vertex(vec3( 0.5f, 0.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.5f, 1.0f, 0.5f), vec3(0,0,0));
    mesh.emplace_vertex(vec3(-0.5f, 1.0f, 0.5f), vec3(0,0,0));
    mesh.push_triangle(0,  1,  2);

    REQUIRE(mesh.get_nv() == 3);
    REQUIRE(mesh.get_ni() == 3);
}

TEST_CASE("Mesh with a single triangle, build normal.", "[mesh]")
{
    Mesh<Vertex3P3N> mesh;
    mesh.emplace_vertex(vec3( 1.0f, 0.0f, 0.0f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 1.0f, 1.0f, 0.0f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.0f, 0.0f, 0.0f), vec3(0,0,0));
    mesh.push_triangle(0, 1, 2);

    mesh.build_normals();

    REQUIRE(mesh[0].get_normal() == vec3(0,0,1));
    REQUIRE(mesh[1].get_normal() == vec3(0,0,1));
    REQUIRE(mesh[2].get_normal() == vec3(0,0,1));
}

TEST_CASE("Mesh with two triangle, build normals and SMOOTH_MAX.", "[mesh]")
{
    Mesh<Vertex3P3N> mesh;
    mesh.emplace_vertex(vec3( 1.0f, 0.0f, 0.0f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 1.0f, 1.0f, 0.0f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.0f, 0.0f, 0.0f), vec3(0,0,0));

    mesh.emplace_vertex(vec3( 1.0f, 0.0f, 0.0f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 0.0f, 0.0f, 0.0f), vec3(0,0,0));
    mesh.emplace_vertex(vec3( 1.0f, 0.0f, 1.0f), vec3(0,0,0));

    mesh.push_triangle(0, 1, 2); // Triangle with normal along z-axis
    mesh.push_triangle(3, 4, 5); // Triangle with normal along y-axis

    mesh.build_normals();
    mesh.smooth_normals();

    // Vertices 0 and 3 are at same position, one with normal=(0,0,1) the other with (0,1,0)
    // Vertices 2 and 4 are at same position, one with normal=(0,0,1) the other with (0,1,0)
    // Smoothing with SMOOTH_MAX will change normals at these points to the mean (0, 0.707107, 0.707107)
    REQUIRE(VectorNear(vec3(0, 0.707107, 0.707107), mesh[0].get_normal(), precision));
    REQUIRE(VectorNear(vec3(0, 0, 1),               mesh[1].get_normal(), precision));
    REQUIRE(VectorNear(vec3(0, 0.707107, 0.707107), mesh[2].get_normal(), precision));
    REQUIRE(VectorNear(vec3(0, 0.707107, 0.707107), mesh[3].get_normal(), precision));
    REQUIRE(VectorNear(vec3(0, 0.707107, 0.707107), mesh[4].get_normal(), precision));
    REQUIRE(VectorNear(vec3(0, 1, 0),               mesh[5].get_normal(), precision));
}

TEST_CASE("Setting up a simple cube mesh.", "[mesh]")
{
    Mesh<Vertex3P3N> mesh;

    init_cube_3P3N(mesh);

    REQUIRE(mesh.get_nv() == 24);
    REQUIRE(mesh.get_ni() == 3*12);
}
