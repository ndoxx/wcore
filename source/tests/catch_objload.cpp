#include <catch2/catch.hpp>

#include "obj_loader.h"
#include "surface_mesh.h"

TEST_CASE("A test objfile is open.", "[obj]")
{
    SurfaceMesh* pmesh = LOADOBJ("../res/models/teapot.obj", true);

    REQUIRE(pmesh->get_nv() == 530);
    REQUIRE(pmesh->get_ni() == 3*992);
}
