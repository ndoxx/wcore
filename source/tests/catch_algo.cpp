#include <catch2/catch.hpp>
#include <iostream>

#include "algorithms.h"
#include "catch_math_common.h"

using namespace math;

TEST_CASE("Previous power of 2.", "[algo]")
{
    REQUIRE(math::pp2(17) == 16);
    REQUIRE(math::pp2(182) == 128);
}

TEST_CASE("Next power of 2.", "[algo]")
{
    REQUIRE(math::np2(17) == 32);
    REQUIRE(math::np2(182) == 256);
}
