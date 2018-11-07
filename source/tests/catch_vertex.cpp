#include <catch2/catch.hpp>
#include <iostream>

#define __DEBUG__

#include <vertex_format.h>

using namespace math;

TEST_CASE("Constructing a Vertex3P2U.", "[vertex]")
{
    Vertex3P2U vertex(vec3(0.0, 0.5, -12.2), vec2(0.5, 0.1));

    std::cout << vertex;

    REQUIRE(vertex.get_position() == vec3(0.0, 0.5, -12.2));
    REQUIRE(vertex.get_uv() == vec2(0.5, 0.1));
}

TEST_CASE("Setting position using rvalue for Vertex3P2U.", "[vertex]")
{
    Vertex3P2U vertex(vec3(0.0, 0.5, -12.2), vec2(0.5, 0.1));

    vertex.set_position(vec3(1.0, -0.5, 44.2));

    REQUIRE(vertex.get_position() == vec3(1.0, -0.5, 44.2));
}

TEST_CASE("Position hash with Vertex3P3N.", "[vertex]")
{
    Vertex3P3N vertex1(vec3(0.0, 1.0, -4.5), vec3(0.0, 1.0, 0.0));
    Vertex3P3N vertex2(vec3(0.0, 1.0000001, -4.5), vec3(0.0, 1.0, 0.0));
    Vertex3P3N vertex3(vec3(0.0, 1.000001, -4.5), vec3(0.0, 1.0, 0.0));

    REQUIRE(vertex1.get_pos_hash() == vertex2.get_pos_hash());
    REQUIRE(vertex1.get_pos_hash() != vertex3.get_pos_hash());
}
