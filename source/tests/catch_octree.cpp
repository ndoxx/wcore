#include <catch2/catch.hpp>
#include <iostream>

#include "octree.hpp"
#include "math3d.h"
#include "camera.h"
#include "catch_math_common.h"

using namespace wcore;

// Will segfault bc of opengl

class PointOctreeFixture
{
public:
    typedef Octree<math::vec3,float> PointOctree;
    typedef PointOctree::content_t   DataList;

    PointOctreeFixture():
    region_({-100,100,0,100,-100,100})
    {
        DataList data_points;
        math::srand_vec3(42);
        for(int ii=0; ii<1000; ++ii)
        {
            math::vec3 point(math::random_vec3(region_.extent));
            data_points.push_back(std::make_pair(point,point.norm()));
        }
        octree_ = new PointOctree(region_, std::move(data_points));
        octree_->subdivide();
    }

    ~PointOctreeFixture()
    {
        delete octree_;
    }

protected:
    BoundingRegion region_;
    PointOctree* octree_;
};

TEST_CASE_METHOD(PointOctreeFixture, "Octree Leaves traversal", "[octree]")
{
    uint32_t npoints=0;
    octree_->traverse_leaves([&](auto&& data)
    {
        ++npoints;
    });

    REQUIRE(npoints == 1000);
}
