#ifndef OCTREE_H
#define OCTREE_H

#include <cstdint>
#include <cassert>
#include <bitset>
#include <functional>
#include <iostream>

#include "math3d.h"

namespace wcore
{

struct BoundingRegion
{
    BoundingRegion() = default;
    BoundingRegion(math::extent_t&& value):
    extent(std::move(value)),
    mid_point(math::vec3((extent[1]+extent[0])*0.5f,
                         (extent[3]+extent[2])*0.5f,
                         (extent[5]+extent[4])*0.5f))
    {

    }

    math::extent_t extent;
    math::vec3 mid_point;
};

template <class content_t>
class Octree
{
public:
    typedef std::function<bool(const content_t&, const BoundingRegion&)> subdivision_predicate_t;

    Octree();
    Octree(const BoundingRegion& bounding_region, content_t&& content);
    ~Octree();

    // Recursive spatial subdivision
    void subdivide(subdivision_predicate_t can_subdivide, Octree* current=nullptr);

    inline void set_node_active(uint8_t index)
    {
        assert(index<8 && "Octree::set_node_active() index out of bounds.");
        active_nodes_[index] = 1;
    }
    inline void set_node_inactive(uint8_t index)
    {
        assert(index<8 && "Octree::set_node_inactive() index out of bounds.");
        active_nodes_[index] = 0;
    }
    inline bool is_node_active(uint8_t index) const
    {
        assert(index<8 && "Octree::is_node_active() index out of bounds.");
        return active_nodes_[index];
    }

private:
    Octree* parent_;
    Octree* children_;
    std::bitset<8> active_nodes_;
    BoundingRegion bounding_region_;
    content_t content_;
};

template <class content_t>
Octree<content_t>::Octree():
parent_(nullptr),
children_(nullptr),
active_nodes_(0x0)
{

}

template <class content_t>
Octree<content_t>::Octree(const BoundingRegion& bounding_region, content_t&& content):
parent_(nullptr),
children_(nullptr),
active_nodes_(0x0),
bounding_region_(bounding_region),
content_(std::move(content))
{

}

template <class content_t>
Octree<content_t>::~Octree()
{
    delete[] children_;
    active_nodes_ = 0x0;
}

template <class content_t>
void Octree<content_t>::subdivide(subdivision_predicate_t can_subdivide, Octree* current)
{
    // * Initial condition
    if(current == nullptr)
        current = this;

    // * Stop condition
    if(!can_subdivide(current->content_, current->bounding_region_))
        return;

    // * Allocate children nodes
    delete[] current->children_;
    current->children_ = new Octree<content_t>[8];

    // * Set children parent to current node
    for(int ii=0; ii<8; ++ii)
    {
        current->children_[ii].parent_ = current;
    }

    // * Subdivide bounding region into octants
    const math::vec3& center = current->bounding_region_.mid_point;
    // Upper octants
    // Octant 0: x>x_c, y>y_c, z>z_c
    current->children_[0].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             center[1], current->bounding_region_.extent[3],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 1: x<x_c, y>y_c, z>z_c
    current->children_[1].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             center[1], current->bounding_region_.extent[3],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 2: x>x_c, y>y_c, z<z_c
    current->children_[2].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             center[1], current->bounding_region_.extent[3],
                                                             current->bounding_region_.extent[4], center[2]});
    // Octant 3: x<x_c, y>y_c, z<z_c
    current->children_[3].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             center[1], current->bounding_region_.extent[3],
                                                             current->bounding_region_.extent[4], center[2]});
    // Lower octants
    // Octant 4: x>x_c, y<y_c, z>z_c
    current->children_[4].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             current->bounding_region_.extent[2], center[1],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 5: x<x_c, y<y_c, z>z_c
    current->children_[5].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             current->bounding_region_.extent[2], center[1],
                                                             center[2], current->bounding_region_.extent[5]});
    // Octant 6: x>x_c, y<y_c, z<z_c
    current->children_[6].bounding_region_ = BoundingRegion({center[0], current->bounding_region_.extent[1],
                                                             current->bounding_region_.extent[2], center[1],
                                                             current->bounding_region_.extent[4], center[2]});
    // Octant 7: x<x_c, y<y_c, z<z_c
    current->children_[7].bounding_region_ = BoundingRegion({current->bounding_region_.extent[0], center[0],
                                                             current->bounding_region_.extent[2], center[1],
                                                             current->bounding_region_.extent[4], center[2]});

    /*for(int ii=0; ii<8; ++ii)
    {
        std::cout << "Octant " << ii << ": ";
        for(int jj=0; jj<6; ++jj)
        {
            std::cout << current->children_[ii].bounding_region_.extent[jj] << " ";
        }
        std::cout << std::endl;
    }*/

    // * Dispatch current content into children octants
    current->content_.traverse([&](const typename content_t::entry_t& entry, const math::vec3& object_position)
    {
        for(int ii=0; ii<8; ++ii)
        {
            // If object position is within child bounding region, add it to child content
            if(object_position[0]>current->children_[ii].bounding_region_.extent[0]
            && object_position[0]<current->children_[ii].bounding_region_.extent[1]
            && object_position[1]>current->children_[ii].bounding_region_.extent[2]
            && object_position[1]<current->children_[ii].bounding_region_.extent[3]
            && object_position[2]>current->children_[ii].bounding_region_.extent[4]
            && object_position[2]<current->children_[ii].bounding_region_.extent[5])
            {
                current->children_[ii].content_.add(entry);
                return;
            }
        }
    });
    current->content_.clear();

    // * Recursively subdivide children nodes
    for(int ii=0; ii<8; ++ii)
    {
        subdivide(can_subdivide, &current->children_[ii]);
    }
}

} // namespace wcore


#endif // OCTREE_H
