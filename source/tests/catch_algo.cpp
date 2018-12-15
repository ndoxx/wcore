#include <catch2/catch.hpp>
#include <iostream>

#include "algorithms.h"
#include "catch_math_common.h"

TEST_CASE("Previous power of 2.", "[algo]")
{
    REQUIRE(pp2(17) == 16);
    REQUIRE(pp2(182) == 128);
}

TEST_CASE("Next power of 2.", "[algo]")
{
    REQUIRE(np2(17) == 32);
    REQUIRE(np2(182) == 256);
}
